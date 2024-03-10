#include "balancer.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

namespace cppfs::balancer {

void Balancer::Run() {
  boost::asio::io_context io_service;

  boost::asio::signal_set signals(io_service, SIGINT, SIGUSR1);

  signals.async_wait(
      [this](boost::system::error_code const& error, int signal_number) {
        if (error) {
          std::cerr << "An error occured when handling the signal\n";
          return;
        }

        std::cout << "Caught signal: " << signal_number << ", bye!\n";
        Stop();
      });

  worker_ = std::jthread([](std::stop_token stop) {
    using namespace std::chrono_literals;

    std::cout << "Started worker thread\n";
    while (true) {
      std::cout << "Working...\n";
      std::this_thread::sleep_for(1s);

      if (stop.stop_requested()) {
        std::cout << "Stopping worker\n";
        break;
      }
    }
  });

  io_service.run();
}

void Balancer::Stop() {
  std::cout << "Stop requested\n";
  worker_.request_stop();
}

}  // namespace cppfs::balancer
