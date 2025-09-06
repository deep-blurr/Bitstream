#include <iostream>
#include <string>
#include <vector>

#include "TorrentParser.hpp"
#include "TrackerClient.hpp"

int main(int argc, char* argv[]){
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_torrent_file>" << std::endl;
        return 1;
    }

    std::string torrent_filepath = argv[1];

    try {
        std::cout << "--- Testing Tracker Client ---" << std::endl;

        std::cout << "[1] Parsing file: " << torrent_filepath << std::endl;
        TorrentFile torrent = parse_torrent_file(torrent_filepath);
        std::cout << "    Announce URL: " << torrent.announce << std::endl;
        std::cout << "    Info Hash:    " << torrent.info_hash_hex << std::endl;

        std::cout << "\n[2] Contacting tracker to get peers..." << std::endl;
        std::vector<PeerInfo> peers = get_peers(torrent);

        if (peers.empty()) {
            std::cout << "\n[SUCCESS] Tracker request was successful, but no peers were returned." << std::endl;
            std::cout << "This is common for torrents that are not actively seeded." << std::endl;
        } else {
            std::cout << "\n[SUCCESS] Recieved " << peers.size() << " peers from the tracker." << std::endl;
            std::cout << "--- Peer List (showing first 10) ---" << std::endl;
            for(size_t i = 0; i < std::min(peers.size(), (size_t)10); ++i) {
                std::cout << "  Peer " << i + 1 << ": " << peers[i].ip << ":" << peers[i].port << std::endl;
            }
            std::cout << "----------------------------------" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "\n[FAILURE] An error occurred: " << std::endl;
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}