#pragma once

#include <thread>

namespace cppfs::balancer {

class Balancer {
 public:
  void Run();
  void Stop();

 private:
  std::jthread worker_;
};

}  // namespace cppfs::balancer
