// ===========================================
// CLIENT_ASYNC.CPP — client TCP asynchrone
// Objectif : se connecter à un serveur, envoyer une ligne, recevoir l’écho
// ===========================================

#include <asio.hpp>      // Bibliothèque Asio
#include <iostream>      // Entrée/sortie standard
#include <string>        // std::string
#include <memory>        // std::shared_ptr

namespace net = asio;
using tcp = net::ip::tcp;

// ---------------------------
// Classe Client : gère une connexion complète côté client
// ---------------------------
struct Client : public std::enable_shared_from_this<Client> {
  // Constructeur : initialise tout
  Client(net::io_context& io, std::string host, std::string port, std::string line)
    : io_(io),                  // Référence au contexte global
      resolver_(io),            // Résolveur DNS
      socket_(io),              // Socket réseau
      host_(std::move(host)),   // Nom d’hôte
      port_(std::move(port))    // Port
  {
    // S’assure que la ligne se termine par '\n'
    if (!line.empty() && line.back() != '\n')
      line.push_back('\n');

    out_ = std::move(line); // Sauvegarde la ligne à envoyer
  }

  // Démarre la séquence de connexion
  void start() {
    // Lance la résolution DNS de host:port
    resolver_.async_resolve(host_, port_,
      [self = shared_from_this()](const net::error_code& ec, tcp::resolver::results_type res) {
        if (ec) { // Erreur DNS
          std::cerr << "[client_async] resolve: " << ec.message() << "\n";
          return;
        }
        // Si ok : passe à l’étape suivante (connexion)
        self->do_connect(std::move(res));
      });
  }

private:
  // Étape 2 : tentative de connexion à un des endpoints résolus
  void do_connect(tcp::resolver::results_type endpoints) {
    net::async_connect(socket_, endpoints,
      [self = shared_from_this()](const net::error_code& ec, const tcp::endpoint&) {
        if (ec) {
          std::cerr << "[client_async] connect: " << ec.message() << "\n";
          return;
        }

        // Option réseau : désactive Nagle
        net::error_code ignore;
        self->socket_.set_option(tcp::no_delay(true), ignore);

        // Étape suivante : envoyer le message
        self->do_write();
      });
  }

  // Étape 3 : envoi du message
  void do_write() {
    net::async_write(socket_, net::buffer(out_),
      [self = shared_from_this()](const net::error_code& ec, std::size_t /*n*/) {
        if (ec) {
          std::cerr << "[client_async] write: " << ec.message() << "\n";
          self->close();
          return;
        }

        // Étape suivante : lire la réponse du serveur
        self->do_read_line();
      });
  }

  // Étape 4 : lecture de la réponse jusqu’à '\n'
  void do_read_line() {
    net::async_read_until(socket_, buf_, '\n',
      [self = shared_from_this()](const net::error_code& ec, std::size_t /*n*/) {
        if (ec) {
          std::cerr << "[client_async] read: " << ec.message() << "\n";
          self->close();
          return;
        }

        // Extraire la ligne reçue
        std::istream is(&self->buf_);
        std::string line;
        std::getline(is, line);
        if (!line.empty() && line.back() == '\r') line.pop_back(); // Nettoie '\r'

        // Affiche la réponse
        std::cout << line << "\n";

        // Ferme la connexion (une seule requête ici)
        self->close();
      });
  }

  // Fermeture propre du socket
  void close() {
    net::error_code ignore;
    socket_.shutdown(tcp::socket::shutdown_both, ignore);
    socket_.close(ignore);
  }

private:
  net::io_context&  io_;     // Référence au contexte global
  tcp::resolver     resolver_; // Pour résoudre host:port
  tcp::socket       socket_;   // Socket client
  std::string       host_, port_; // Coordonnées serveur
  std::string       out_;        // Message à envoyer
  net::streambuf    buf_;        // Buffer de réception
};

// ---------------------------
// main() : point d’entrée du client
// ---------------------------
int main(int argc, char** argv) {
  try {
    // Récupère hôte et port depuis la ligne de commande
    std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    std::string port = (argc > 2) ? argv[2] : "5555";

    // Lit une ligne sur stdin
    std::string line;
    if (!std::getline(std::cin, line)) {
      std::cerr << "[client_async] no input on stdin\n";
      return 2;
    }

    // Crée le contexte d’exécution
    net::io_context io;

    // Crée et démarre un client
    auto client = std::make_shared<Client>(io, host, port, line);
    client->start();

    // Lance la boucle d’événements (bloquante)
    io.run();

  } catch (const std::exception& ex) {
    // Gestion globale d’erreurs (exceptions Asio, etc.)
    std::cerr << "[client_async] fatal: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
