// ===========================================
// CLIENT_SYNC.CPP
// Client TCP synchrone minimal avec Asio (standalone)
// Objectif : se connecter à 127.0.0.1:5555, lire une ligne sur stdin,
//            l'envoyer avec '\n', puis afficher la réponse serveur.
// ===========================================

#include <asio.hpp>     // Asio header-only
#include <iostream>     // logs + std::cout/cerr
#include <string>       // std::string

namespace net = asio;
using tcp = net::ip::tcp;

int main(int argc, char** argv) {
  try {
    // 1) Paramètres : hôte et port (defaults pour le loopback local)
    std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    std::string port = (argc > 2) ? argv[2] : "5555";

    // 2) Contexte I/O Asio
    net::io_context io;

    // 3) Résolution (gère IP directe ou nom DNS)
    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(host, port); // peut renvoyer plusieurs endpoints

    // 4) Socket cliente
    tcp::socket sock(io);

    // 5) Connexion bloquante au premier endpoint valide
    //    Sous le capot : essais successifs de connect() si plusieurs endpoints.
    net::connect(sock, endpoints);

    // 6) Lire une ligne sur stdin (message à envoyer)
    //    On utilise un protocole "ligne" : le serveur lit jusqu'au '\n'.
    std::string line;
    if (!std::getline(std::cin, line)) {
      std::cerr << "[client] no input on stdin\n";
      return 2; // code de retour explicite si pas d'entrée
    }
    line.push_back('\n'); // important : terminer par '\n' (framing)

    // 7) Envoyer la ligne (bloquant)
    asio::error_code ec;
    net::write(sock, net::buffer(line), ec);
    if (ec) {
      std::cerr << "[client] write error: " << ec.message() << "\n";
      return 3;
    }

    // 8) Lire la réponse jusqu'à '\n' (bloquant)
    net::streambuf buf;
    std::size_t n = net::read_until(sock, buf, '\n', ec);
    (void)n; // on ignore n ici ; utile plus tard pour logs/metrics
    if (ec) {
      std::cerr << "[client] read_until error: " << ec.message() << "\n";
      return 4;
    }

    // 9) Extraire et afficher la ligne reçue (sans le '\n')
    std::istream is(&buf);
    std::string resp;
    std::getline(is, resp);
    std::cout << resp << "\n";

    // 10) Fermeture propre
    net::error_code ignore;
    sock.shutdown(tcp::socket::shutdown_both, ignore);
    sock.close(ignore);

  } catch (const std::exception& ex) {
    std::cerr << "[client] fatal: " << ex.what() << "\n";
    return 1;
  }
  return 0;
}