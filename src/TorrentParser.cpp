#include "TorrentParser.hpp"

#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>

// Reads data from filepath(torrent file)
static std::string load_file_content(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + filepath);
  }

  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// Calculate SHA1 hash (needed for info hash)
static std::string calculate_hash(const std::string &input) {
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char *>(input.c_str()), input.length(),
       hash);
  return std::string(reinterpret_cast<char *>(hash), SHA_DIGEST_LENGTH);
}

static std::string to_hex_string(const std::string &bytes) {
  std::stringstream hex_stream;
  hex_stream << std::hex << std::setfill('0');
  for (unsigned char c : bytes) {
    hex_stream << std::setw(2) << static_cast<int>(c);
  }
  return hex_stream.str();
}

TorrentFile parse_torrent_file(const std::string &filepath) {
  try {
    // Load file content
    std::string bencoded_content = load_file_content(filepath);
    bencode::data decoded_data = bencode::decode(bencoded_content);
    auto &root_dict = std::get<bencode::dict>(decoded_data);

    TorrentFile torrent;

    // Extract announce and info dict
    torrent.announce = std::get<bencode::string>(root_dict.at("announce"));
    bencode::data info_data_node = root_dict.at("info");
    torrent.info = std::get<bencode::dict>(info_data_node);

    // Reencode info dict for sha1
    std::string info_bencoded_str = bencode::encode(info_data_node);
    torrent.raw_info_hash = calculate_hash(info_bencoded_str);
    torrent.info_hash_hex = to_hex_string(torrent.raw_info_hash);

    // Extrct metadata
    torrent.piece_length =
        std::get<bencode::integer>(torrent.info.at("piece length"));
    torrent.pieces = std::get<bencode::string>(torrent.info.at("pieces"));

    if (torrent.info.count("length")) {
      torrent.total_length =
          std::get<bencode::integer>(torrent.info.at("length"));
    } else if (torrent.info.count("files")) {
      torrent.total_length = 0;
      auto &files_list = std::get<bencode::list>(torrent.info.at("files"));
      for (const auto &file_item : files_list) {
        auto &file_dict = std::get<bencode::dict>(file_item);
        torrent.total_length +=
            std::get<bencode::integer>(file_dict.at("length"));
      }
    } else {
      throw std::runtime_error(
          "Torrent file missing 'length' or 'files' key in info dictionary.");
    }

    return torrent;
  } catch (const bencode::decode_error &e) {
    throw std::runtime_error("Bencode decoding error in '" + filepath +
                             "' at offset " + std::to_string(e.offset()) +
                             ": " + e.what());
  } catch (const std::bad_variant_access &e) {
    throw std::runtime_error(
        "Type error: Unexpected data structure in torrent file: '" + filepath +
        "'.");
  } catch (const std::out_of_range &e) {
    throw std::runtime_error(
        "Format error: Missing required key in torrent dictionary: '" +
        filepath + "'.");
  }
}
