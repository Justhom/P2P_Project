// ===========================================
// SERVER_SYNC.CPP
// Serveur TCP synchrone minimal avec Asio
// Objectif : écouter sur le port 5555, recevoir un message texte terminé par '\n'
//             et renvoyer "# echo> <message>"
// ===========================================

#include <asio.hpp>     // Librairie réseau C++ moderne (standalone, sans Boost)
#include <iostream>     // Pour afficher des logs dans le terminal
#include <string>       // Pour manipuler des chaînes de caractères

// Pour raccourcir les noms (plutôt que asio::ip::tcp, on écrira tcp)
namespace net = asio;
using tcp = net::ip::tcp;

int main() {
  try {
    // -------------------------------------------
    // 1️⃣ Création du moteur d'E/S réseau
    // -------------------------------------------
    // io_context gère toutes les opérations réseau : ouverture de socket, acceptation, lecture, écriture.
    // Même en mode synchrone, Asio a besoin d'un contexte d'I/O.
    net::io_context io;

    // -------------------------------------------
    // 2️⃣ Création d’un "acceptor" (porte d’entrée du serveur)
    // -------------------------------------------
    // tcp::v4()  → on écoute sur toutes les interfaces IPv4 locales (0.0.0.0)
    // Port 5555  → choisi arbitrairement, non privilégié (>1024)
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 5555));

    std::cout << "[server] listening on 0.0.0.0:5555\n";

    // -------------------------------------------
    // 3️⃣ Boucle principale : accepter plusieurs clients successifs
    // -------------------------------------------
    // Tant que le programme tourne, on accepte une connexion, on la traite, puis on recommence.
    for (;;) {
      net::error_code ec;       // Stocke les erreurs sans lancer d'exception
      tcp::socket sock(io);     // Socket vide pour accueillir un client

      // -------------------------------------------
      // 4️⃣ Attente bloquante d’un client
      // -------------------------------------------
      // Cette ligne bloque jusqu’à ce qu’un client se connecte sur le port 5555.
      acceptor.accept(sock, ec);

      if (ec) {
        std::cerr << "[server] accept error: " << ec.message() << "\n";
        continue; // On retourne écouter sans planter le serveur
      }

      // -------------------------------------------
      // 5️⃣ Log de la connexion entrante
      // -------------------------------------------
      // remote_endpoint() donne l’adresse et le port du client connecté
      std::cout << "[server] client: " << sock.remote_endpoint(ec) << "\n";

      // -------------------------------------------
      // 6️⃣ Lecture du message du client
      // -------------------------------------------
      // On lit dans la socket jusqu’à recevoir un caractère '\n'.
      // Cela définit un protocole simple : chaque message est une ligne.
      net::streambuf buf; // tampon interne de réception

      std::size_t n = net::read_until(sock, buf, '\n', ec);
      // Cette opération est BLOQUANTE :
      //   → si le client ne finit pas par '\n', le serveur attendra indéfiniment.

      if (ec) {
        std::cerr << "[server] read_until error: " << ec.message() << "\n";
      } else {
        // -------------------------------------------
        // 7️⃣ Extraction de la ligne lue du tampon
        // -------------------------------------------
        std::istream is(&buf);  // Crée un flux de lecture à partir du tampon
        std::string line;
        std::getline(is, line); // Lit la ligne sans le '\n'

        // -------------------------------------------
        // 8️⃣ Préparation de la réponse
        // -------------------------------------------
        std::string out = "# echo> " + line + "\n";

        // -------------------------------------------
        // 9️⃣ Envoi de la réponse
        // -------------------------------------------
        // net::buffer() crée un tampon mémoire sur la chaîne
        // net::write() écrit tous les octets sur la socket (bloquant aussi)
        net::write(sock, net::buffer(out), ec);

        if (ec) {
          std::cerr << "[server] write error: " << ec.message() << "\n";
        } else {
          std::cout << "[server] replied: " << out;
        }
      }

      // -------------------------------------------
      // 🔟 Fermeture propre de la connexion
      // -------------------------------------------
      // Toujours fermer proprement :
      // - shutdown() pour dire “j’ai fini d’envoyer et recevoir”
      // - close() pour libérer la ressource
      net::error_code ignore;
      sock.shutdown(tcp::socket::shutdown_both, ignore);
      sock.close(ignore);

      std::cout << "[server] connection closed\n";
    }

  } catch (const std::exception& ex) {
    // -------------------------------------------
    // 🔥 Gestion des exceptions générales
    // -------------------------------------------
    // Par exemple : port déjà pris, permission refusée, etc.
    std::cerr << "[server] fatal: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}