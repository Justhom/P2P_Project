// ===========================================
// SERVER_ASYNC.CPP — serveur TCP asynchrone (Asio standalone)
// Objectif : écouter sur un port, accepter plusieurs clients simultanément,
// lire des messages terminés par '\n' et renvoyer "# echo> <message>\n"
// ===========================================

#include <asio.hpp>      // Bibliothèque réseau Asio (header-only)
#include <iostream>      // Pour les logs console
#include <string>        // Pour manipuler les chaînes
#include <memory>        // Pour std::shared_ptr

// Alias pour éviter d’écrire asio:: partout
namespace net = asio;
using tcp = net::ip::tcp;

// Définition de constantes globales
static constexpr unsigned short LISTEN_PORT = 5555;              // Port d’écoute
static constexpr std::size_t MAX_LINE = 64 * 1024;               // Taille max d’un message (64 KiB)
static constexpr std::chrono::seconds READ_TIMEOUT{15};          // Timeout lecture : 15 secondes

// ---------------------------
// Classe Session : représente une connexion client individuelle
// ---------------------------
class Session : public std::enable_shared_from_this<Session> {
public:
  // Constructeur : on reçoit une socket déjà connectée
  explicit Session(tcp::socket socket)
    : socket_(std::move(socket)),       // On prend possession de la socket
      timer_(socket_.get_executor())    // Timer associé au même contexte d’exécution que la socket
  {}

  // Fonction publique appelée juste après l’acceptation
  void start() {
    net::error_code ec;
    socket_.set_option(tcp::no_delay(true), ec); // Désactive Nagle (améliore latence petits messages)
    (void)ec;                                    // Ignore les erreurs mineures

    do_read_line();   // Lance la première lecture asynchrone
    arm_timeout();    // Active le timer d’inactivité
  }

private:
  // Lecture asynchrone jusqu’à '\n'
  void do_read_line() {
    auto self = shared_from_this(); // Garde la session en vie pendant l’opération

    // Protection anti-flood : si le buffer dépasse la taille max, on coupe
    if (buffer_.size() > MAX_LINE) {
      close();
      return;
    }

    // Lance une lecture asynchrone qui se termine quand un '\n' est reçu
    net::async_read_until(socket_, buffer_, '\n',
      [this, self](const net::error_code& ec, std::size_t /*n*/) {
        // Callback appelé quand la lecture est terminée (ou erreur)
        cancel_timeout(); // Annule le timer (on a reçu des données)

        if (ec) { // Si erreur (EOF, déconnexion, etc.)
          close(); // Ferme proprement la session
          return;
        }

        // Extraire la ligne du buffer
        std::istream is(&buffer_); // Crée un flux pour lire dans le buffer
        std::string line;
        std::getline(is, line);    // Lit jusqu’à '\n' (le retire)
        if (!line.empty() && line.back() == '\r') line.pop_back(); // Supprime un éventuel '\r'

        // Prépare la réponse "# echo> <message>\n"
        out_ = "# echo> " + line + "\n";

        // Envoie la réponse asynchrone
        do_write();
      }
    );
  }

  // Écriture asynchrone de la réponse
  void do_write() {
    auto self = shared_from_this(); // Garde l’objet vivant pendant l’envoi

    // async_write envoie tous les octets du buffer (asynchrone)
    net::async_write(socket_, net::buffer(out_),
      [this, self](const net::error_code& ec, std::size_t /*n*/) {
        if (ec) { // Si erreur pendant écriture (client fermé, etc.)
          close();
          return;
        }

        // Sinon : relance un cycle de lecture
        arm_timeout();  // Redémarre le timer
        do_read_line(); // Attend un nouveau message
      }
    );
  }

  // Arme un timer d’inactivité (timeout)
  void arm_timeout() {
    auto self = shared_from_this();
    timer_.expires_after(READ_TIMEOUT); // Définit l’échéance du timer

    // Lance une attente asynchrone
    timer_.async_wait([this, self](const net::error_code& ec) {
      if (ec) return; // Si timer annulé, on ne fait rien
      close();        // Sinon : timeout -> on ferme la connexion
    });
  }

  // Annule le timer actuel
  void cancel_timeout() {
    timer_.cancel(); // Annule sans exception
  }

  // Ferme proprement la connexion
  void close() {
    cancel_timeout(); // Stoppe le timer
    net::error_code ignore;
    socket_.shutdown(tcp::socket::shutdown_both, ignore); // Ferme la lecture/écriture
    socket_.close(ignore); // Libère la ressource
  }

private:
  tcp::socket socket_;         // Socket de la connexion
  net::streambuf buffer_;      // Tampon pour lecture
  std::string out_;            // Message à envoyer
  net::steady_timer timer_;    // Timer d’inactivité
};

// ---------------------------
// Fonction principale : création du serveur et gestion des acceptations
// ---------------------------
int main() {
  try {
    net::io_context io; // Boucle d’événements principale

    // Création de l’acceptor : écoute sur 0.0.0.0:5555
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), LISTEN_PORT));

    // Permet de relancer le serveur immédiatement après un crash sans "port déjà pris"
    acceptor.set_option(net::socket_base::reuse_address(true));

    std::cout << "[server_async] listening on 0.0.0.0:" << LISTEN_PORT << "\n";

    // Déclare une lambda récursive pour gérer l’acceptation en boucle
    std::function<void()> do_accept;

    do_accept = [&]() {
      // async_accept lance une attente non bloquante d’un client entrant
      acceptor.async_accept(
        [&](const net::error_code& ec, tcp::socket socket) {
          // Callback quand une connexion est acceptée
          if (!ec) {
            auto ep = socket.remote_endpoint();
            std::cout << "[server_async] client " << ep << "\n";

            // Crée et démarre une session pour ce client
            std::make_shared<Session>(std::move(socket))->start();
          }
          // Relance immédiatement l’attente d’un autre client
          do_accept();
        }
      );
    };

    do_accept(); // Démarre la première attente
    io.run();    // Lance la boucle d’événements (bloquante)

  } catch (const std::exception& ex) {
    // Gestion des erreurs globales (ex: port déjà pris)
    std::cerr << "[server_async] fatal: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
