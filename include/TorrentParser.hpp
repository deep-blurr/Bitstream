#pragma once

#include "bencode.hpp"
#include <string>
#include <vector>

struct TorrentFile {
  std::string announce;
  bencode::dict info;

  std::string raw_info_hash;
  std::string info_hash_hex;

  long long piece_length;
  long long total_length;
  std::string pieces;
};

TorrentFile parse_torrent_file(const std::string &filepath);
