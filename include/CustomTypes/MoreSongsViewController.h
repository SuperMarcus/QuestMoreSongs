#pragma once

#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"
#include "custom-types/shared/register.hpp"
#include "custom-types/shared/macros.hpp"

#include "HMUI/ViewController.hpp"

DECLARE_CLASS_CODEGEN(moresongs::CustomTypes, MoreSongsViewController, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD(void, DidDeactivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidDeactivate", 2), bool removedFromHierarchy, bool screenSystemDisabling);

private:
    void onSearchInputUpdate(std::string data);

public:
    REGISTER_FUNCTION(MoreSongsViewController,
        REGISTER_METHOD(DidActivate);
        REGISTER_METHOD(DidDeactivate);
    )
)
