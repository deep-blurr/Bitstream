#include "TorrentParser.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_torrent_file>" << std::endl;
    return 1;
  }

  std::string torrent_file_path = argv[1];

  try {
    std::cout << "--- Testing Torrent Parser ---" << std::endl;
    std::cout << "Parsing file:" << torrent_file_path << std::endl;

    TorrentFile torrent = parse_torrent_file(torrent_file_path);

    std::cout << "\n[SUCCESS] Torrent file parsed successfully. \n"
              << std::endl;
    std::cout << "--- Parsed Data ---" << std::endl;
    std::cout << "Announce URL:    " << torrent.announce << std::endl;
    std::cout << "Info Hash (Hex): " << torrent.info_hash_hex << std::endl;
    std::cout << "Total Length:    " << torrent.total_length << " bytes"
              << std::endl;
    std::cout << "Piece Length:    " << torrent.piece_length << " bytes"
              << std::endl;

    if (torrent.pieces.length() % 20 != 0) {
      std::cerr
          << "[WARNING] The 'pieces' string length is not a multiple of 20. "
          << std::endl;
    }
    long long num_pieces = torrent.pieces.length() / 20;
    std::cout << "Number of Pieces:" << num_pieces << std::endl;
    std::cout << "------------------" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "\n[FAILURE] An error occurred during parsing:" << std::endl;
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
