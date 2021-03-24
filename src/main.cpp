#include "utils/il2cpp-utils.hpp"
#include "utils/utils.h"
#include "modloader.hpp"
#include "moresongs_private.h"

#include "QuestUI.hpp"
#include "custom-types/shared/register.hpp"

#include "CustomTypes/MoreSongsViewController.h"

using namespace moresongs;

static ModInfo modInfo;

Logger& moresongs::getLogger() {
    static auto logger = new Logger(modInfo);
    return *logger;
}

ModInfo& moresongs::getModInfo() {
    return modInfo;
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
    getLogger().info("Setting up More Songs.");
}

extern "C" void load() {
    getLogger().info("Loading More Songs...");
    QuestUI::Init();
    custom_types::Register::RegisterTypes<moresongs::CustomTypes::MoreSongsViewController>();
    QuestUI::Register::RegisterModSettingsViewController<moresongs::CustomTypes::MoreSongsViewController*>(getModInfo(), "More Songs");
    getLogger().info("Finished loading More Songs.");
}
