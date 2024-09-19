#pragma once

#include <filesystem>
#include <string>

namespace cppfs::storage {

int StartFS(std::string const& host, int port,
            std::filesystem::path const& cert,
            std::filesystem::path const& key);
}
