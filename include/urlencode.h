#pragma once

#include <string>

namespace moresongs {
    std::string decodeURIComponent(std::string encoded);
    std::string encodeURIComponent(const std::string& decoded);
}
