#include <CLI/CLI.hpp>
#include <iostream>
#include <string>

#include "server/server.hpp"

struct Config {
  std::string ip_address;
  int port;
  std::string cert_path;
  std::string key_path;
};

Config parse_arguments(int argc, char** argv) {
  CLI::App app{"SSL Server Configuration"};

  Config config;

  app.add_option("-a,--address", config.ip_address, "IP Address to bind to")
      ->required()
      ->check(CLI::ValidIPV4);

  app.add_option("-p,--port", config.port, "Port to bind to")
      ->required()
      ->check(CLI::Range(1, 65535));

  app.add_option("-c,--cert", config.cert_path, "Path to SSL certificate")
      ->required()
      ->check(CLI::ExistingFile);

  app.add_option("-k,--key", config.key_path, "Path to SSL key")
      ->required()
      ->check(CLI::ExistingFile);

  app.parse(argc, argv);

  return config;
}

int main(int argc, char** argv) {
  try {
    Config config = parse_arguments(argc, argv);

    std::cout << "IP Address: " << config.ip_address << "\n";
    std::cout << "Port: " << config.port << "\n";
    std::cout << "Certificate Path: " << config.cert_path << "\n";
    std::cout << "Key Path: " << config.key_path << "\n";
    cppfs::storage::StartFS(config.ip_address, config.port, config.cert_path,
                            config.key_path);
  } catch (const CLI::ParseError& e) {
    return CLI::App().exit(e);  // Handles parsing errors
  }

  return 0;
}
