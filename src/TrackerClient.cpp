#define CPPHTTPLIB_OPENSSL_SUPPORT

#include "TrackerClient.hpp"
#include "bencode.hpp"
#include "httplib.h"

#include <arpa/inet.h>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

static std::string generate_peer_id() {
  std::string peer_id = "-TR0001-";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, 255);
  for (size_t i = 0; i < 12; ++i) {
    peer_id += static_cast<char>(distrib(gen));
  }
  return peer_id;
}

static std::string url_encode(const std::string &data) {
  std::stringstream encoded;
  encoded << std::hex << std::uppercase << std::setfill('0');
  for (unsigned char c : data) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded << c;
    } else {
      encoded << '%' << std::setw(2) << static_cast<int>(c);
    }
  }
  return encoded.str();
}

static std::vector<PeerInfo> parse_compact_peers(const std::string &peers_str) {
  std::vector<PeerInfo> peers;
  if (peers_str.length() % 6 != 0) {
    throw std::runtime_error("Invalid compact peers list length.");
  }

  for (size_t i = 0; i < peers_str.length(); i += 6) {
    uint32_t ip_net_order;
    std::memcpy(&ip_net_order, peers_str.data() + i, 4);
    uint32_t ip_host_order = ntohl(ip_net_order);

    in_addr ip_addr;
    ip_addr.s_addr = ip_net_order;
    std::string ip_str = inet_ntoa(ip_addr);

    uint16_t port_net_order;
    std::memcpy(&port_net_order, peers_str.data() + i + 4, 2);
    uint16_t port_host_order = ntohs(port_net_order);

    peers.push_back({ip_str, port_host_order});
  }
  return peers;
}

template <typename ClientType>
static httplib::Result perform_tracker_request(ClientType &cli,
                                               const std::string &full_path) {
  cli.set_connection_timeout(20);
  cli.set_default_headers({{"User-Agent", "BitTorrentClient/0.0.1"}});
  return cli.Get(full_path.c_str());
}

std::vector<PeerInfo> get_peers(const TorrentFile &torrent) {

  std::string peer_id = generate_peer_id();
  std::string encoded_info_hash = url_encode(torrent.raw_info_hash);
  std::string encoded_peer_id = url_encode(peer_id);

  const std::string &url_str = torrent.announce;
  std::string scheme, host_port, path;

  size_t scheme_end = url_str.find("://");
  if (scheme_end == std::string::npos) {
    throw std::runtime_error(
        "Invalid announce URL: missing scheme (http/https).");
  }
  scheme = url_str.substr(0, scheme_end);

  size_t host_start = scheme_end + 3;
  size_t path_start = url_str.find('/', host_start);

  if (path_start == std::string::npos) {
    host_port = url_str.substr(host_start);
    path = "/";
  } else {
    host_port = url_str.substr(host_start, path_start - host_start);
    path = url_str.substr(path_start);
  }

  std::stringstream query_stream;
  query_stream << path << "?info_hash=" << encoded_info_hash
               << "&peer_id=" << encoded_peer_id << "&port=6881"
               << "&uploaded=0"
               << "&downloaded=0"
               << "&left=" << torrent.total_length << "&compact=1";
  std::string full_path = query_stream.str();

  httplib::Result res;
  if (scheme == "https") {
    httplib::SSLClient cli(host_port);
    res = perform_tracker_request(cli, full_path);
  } else if (scheme == "http") {
    httplib::Client cli(host_port);
    res = perform_tracker_request(cli, full_path);
  } else {
    throw std::runtime_error("Unsupported scheme: '" + scheme +
                             "'. Only http and https are supported.");
  }

  if (!res) {
    auto err = res.error();
    throw std::runtime_error("HTTP request to tracker failed: " +
                             httplib::to_string(err));
  }

  if (res->status != 200) {
    throw std::runtime_error("Tracker returned non-200 status: " +
                             std::to_string(res->status) + " - " + res->body);
  }

  bencode::data tracker_response = bencode::decode(res->body);
  auto &response_dict = std::get<bencode::dict>(tracker_response);

  if (response_dict.count("failure reason")) {
    throw std::runtime_error(
        "Tracker error: " +
        std::get<bencode::string>(response_dict.at("failure reason")));
  }

  if (!response_dict.count("peers")) {
    throw std::runtime_error("Tracker response missing 'peers' key.");
  }

  const auto &peers_val = response_dict.at("peers");
  if (std::holds_alternative<bencode::string>(peers_val)) {
    // This is the compact format we expect
    return parse_compact_peers(std::get<bencode::string>(peers_val));
  } else if (std::holds_alternative<bencode::list>(peers_val)) {
    // This is the non-compact format, or an empty list for an unknown torrent.
    // For our MVP, we will consider this a successful response with 0 peers.
    // A full client would parse this list of dictionaries.
    return {}; // Return an empty vector of peers
  } else {
    throw std::runtime_error("Unexpected data type for 'peers' key.");
  }
}
