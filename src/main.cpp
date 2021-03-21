#include "modloader.hpp"
#include "moresongs_private.h"

static ModInfo modInfo;

Logger& getLogger() {
    static auto logger = new Logger(modInfo);
    return *logger;
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
    getLogger().info("Setting up More Songs.");
}

extern "C" void load() {
    getLogger().info("Loading More Songs...");
    getLogger().info("Finished loading More Songs.");
}
