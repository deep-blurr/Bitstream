#pragma once

#include "TorrentParser.hpp"
#include <cstdint>
#include <string>
#include <vector>

struct PeerInfo {
  std::string ip;
  uint16_t port;
};

std::vector<PeerInfo> get_peers(const TorrentFile &torrent);
