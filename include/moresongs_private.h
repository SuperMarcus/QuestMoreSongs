#pragma once

#include "utils/utils.h"
#include "config/config-utils.hpp"
#include "utils/logging.hpp"

namespace moresongs {
    ModInfo& getModInfo();
    Logger& getLogger();
}

namespace moresongs::literals {
    inline Il2CppString* operator "" _cs(const char *cString, size_t length) {
        return il2cpp_utils::createcsstr(std::basic_string_view<char>(cString, length));
    }

    inline Il2CppString* operator "" _csP(const char *cString, size_t length) {
        return il2cpp_utils::createcsstr(std::basic_string_view<char>(cString, length), il2cpp_utils::StringType::Manual);
    }
}
