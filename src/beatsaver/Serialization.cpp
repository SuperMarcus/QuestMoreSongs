#include "BeatSaver.h"
#include "utils/il2cpp-utils.hpp"
#include "moresongs_private.h"

using namespace moresongs;
using namespace rapidjson;

BeatSaverSong::BeatSaverSong(const ConstJsonObject & songJson)
    : metadata(songJson["metadata"].GetObject()) {
    key = songJson["key"].GetString();
    name = songJson["name"].GetString();
    description = songJson["description"].GetString();
    hash = songJson["hash"].GetString();
    coverURL = songJson["coverURL"].GetString();
}

BeatSaverSongDifficulties::BeatSaverSongDifficulties(const ConstJsonObject & obj) {
    easy = obj["easy"].GetBool();
    normal = obj["normal"].GetBool();
    hard = obj["hard"].GetBool();
    expert = obj["expert"].GetBool();
    expertPlus = obj["expertPlus"].GetBool();
}

BeatSaverSongMetadata::BeatSaverSongMetadata(const ConstJsonObject & obj)
    : difficulties(obj["difficulties"].GetObject()) {
    duration = obj["duration"].GetInt();
    bpm = obj["bpm"].GetInt();
    levelAuthorName = obj["levelAuthorName"].GetString();
    songAuthorName = obj["songAuthorName"].GetString();
    songName = obj["songName"].GetString();
    songSubName = obj["songSubName"].GetString();

    auto charArr = obj["characteristics"].GetArray();
    for (const auto& charObjE : charArr) {
        characteristics.emplace_back(charObjE.GetObject());
    }
}

BeatSaverSongCharacteristic::BeatSaverSongCharacteristic(const ConstJsonObject & obj)
    : difficulties(obj["difficulties"].GetObject()) {
    name = obj["name"].GetString();
}

BeatSaverPage::BeatSaverPage(const ConstJsonObject & obj) {
    auto docArr = obj["docs"].GetArray();
    for (const auto& docObjE : docArr) {
        docs.emplace_back(docObjE.GetObject());
    }

    totalDocs = obj["totalDocs"].GetInt();
    lastPage = obj["lastPage"].GetInt();

    if (!obj["prevPage"].IsNull()) {
        prevPage = obj["prevPage"].GetInt();
    }

    if (!obj["nextPage"].IsNull()) {
        nextPage = obj["nextPage"].GetInt();
    }
}

BeatSaverSongCharacteristicDifficultyData::BeatSaverSongCharacteristicDifficultyData(const ConstJsonObject & obj) {
    duration = obj["duration"].GetFloat();
    length = obj["length"].GetInt();
    bombs = obj["bombs"].GetInt();
    notes = obj["notes"].GetInt();
    obstacles = obj["obstacles"].GetInt();
    njs = obj["njs"].GetInt();
    njsOffset = obj["njsOffset"].GetInt();
}

BeatSaverSongCharacteristicDifficulties::BeatSaverSongCharacteristicDifficulties(const ConstJsonObject& obj) {
    if (obj["easy"].IsObject()) {
        easy.emplace(obj["easy"].GetObject());
    }

    if (obj["normal"].IsObject()) {
        normal.emplace(obj["normal"].GetObject());
    }

    if (obj["hard"].IsObject()) {
        hard.emplace(obj["hard"].GetObject());
    }

    if (obj["expert"].IsObject()) {
        expert.emplace(obj["expert"].GetObject());
    }

    if (obj["expertPlus"].IsObject()) {
        expertPlus.emplace(obj["expertPlus"].GetObject());
    }
}
