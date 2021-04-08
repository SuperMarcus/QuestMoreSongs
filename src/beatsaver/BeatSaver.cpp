#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

#include "utils/utils.h"
#include "utils/il2cpp-utils.hpp"
#include "config/config-utils.hpp"
#include "config/rapidjson-utils.hpp"
#include "custom-types/shared/coroutine.hpp"

#include "rapidjson/include/rapidjson/error/en.h"
#include "rapidjson/include/rapidjson/encodedstream.h"

#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/DownloadHandler.hpp"
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"
#include "UnityEngine/AsyncOperation.hpp"
#include "UnityEngine/YieldInstruction.hpp"
#include "GlobalNamespace/SharedCoroutineStarter.hpp"
#include "System/Action_1.hpp"
#include "System/Collections/IEnumerator.hpp"

#include "songloader/shared/API.hpp"

#include "BeatSaver.h"
#include "moresongs_private.h"
#include "zip.h"
#include "urlencode.h"

#include <utility>
#include <string_view>
#include <fstream>
#include <exception>
#include <future>

using namespace std::literals;
using namespace moresongs;
using namespace moresongs::literals;
using namespace UnityEngine;
using namespace UnityEngine::Networking;
using namespace System;
using namespace System::Threading;
using namespace custom_types::Helpers;

using Coro = custom_types::Helpers::Coroutine;

constexpr auto beatsaverDefaultEndpoint = "https://beatsaver.com/api"sv;
constexpr auto downloadUrlFormat = "%s/download/key/%s"sv;
constexpr auto mapsDetailByKeyUrlFormat = "%s/maps/detail/%s"sv;
constexpr auto pageUrlFormat = "%s%s%d"sv;
constexpr auto searchUrlFormat = "%s/search/text/%d?q=%s"sv;

static Il2CppString *userAgent;

// A generic request maker
template<typename ResultType, typename... AdditionalArgs>
static void StartGenericRequestCoro(const std::string& requestUrl, CallbackFunc<ResultType> handler) {
    auto co = CoroutineHelper::New(([&](CallbackFunc<ResultType> handler) -> Coro {
        // Setup request
        auto request = UnityWebRequest::Get(il2cpp_utils::createcsstr(requestUrl));
        request->SetRequestHeader("User-Agent"_cs, userAgent);

        // Send request
        co_yield reinterpret_cast<enumeratorT*>(request->SendWebRequest());

        // Parse response
        std::optional<ResultType> result {};
        auto responseCode = request->get_responseCode();

        if (responseCode == 200) {
            rapidjson::Document responseDoc {};
            auto responseText = to_utf8(csstrtostr(request->get_downloadHandler()->GetText()));

            // Sanitize response string
            auto nInvalidChars = 0;
            for (auto& c : responseText) {
                if (static_cast<unsigned char>(c) < 0x20 || static_cast<unsigned char>(c) > 127) {
                    c = '?';
                    ++nInvalidChars;
                }
            }

            if (nInvalidChars > 0) {
                getLogger().warning("%d invalid characters found in response.", nInvalidChars);
            }

            // Parse the response json
            rapidjson::ParseResult parseResult = responseDoc.Parse(responseText);

            if (parseResult) {
                auto songObj = static_cast<const rapidjson::Document&>(responseDoc).GetObject();
                result.emplace(std::move(songObj));
            } else {
                getLogger().warning("RapidJson unable to parse response: %s (%u: %X)", rapidjson::GetParseError_En(parseResult.Code()), parseResult.Offset(), responseText[parseResult.Offset()]);
            }
        } else {
            getLogger().warning("BeatSaver returned an error status code from request: %ld", responseCode);
        }

        handler(std::move(result));
    })(std::move(handler)));
    auto coStarter = GlobalNamespace::SharedCoroutineStarter::get_instance();
    coStarter->StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(co));
}

moresongs::SongDownloadingTaskPtr moresongs::BeatSaver::downloadSongWithKey(const moresongs::BeatSaverKey& key) {
    if (downloadTasks.contains(key)) {
        return downloadTasks[key];
    }

    auto downloadTask = std::make_shared<SongDownloadingTask>(key);
    downloadTasks[key] = downloadTask;

    // Begin downloading the song
    auto downloadUrl = string_format(downloadUrlFormat, endpoint.data(), key.c_str());
    downloadTask->webRequest = UnityEngine::Networking::UnityWebRequest::Get(il2cpp_utils::createcsstr(downloadUrl));

    // Request parameters
    downloadTask->webRequest->SetRequestHeader("User-Agent"_cs, userAgent);

    // Forward response to task
    auto requestOp = downloadTask->webRequest->SendWebRequest();
    requestOp->add_completed(il2cpp_utils::MakeDelegate<System::Action_1<AsyncOperation*>*>(
        classof(System::Action_1<AsyncOperation*>*),
        static_cast<std::function<void(AsyncOperation*)>>([downloadTask](AsyncOperation* op) mutable {
            if (downloadTask) {
                downloadTask->onDownloadTaskCompletion();
                downloadTask.reset();
            }
        })
    ));

    return downloadTask;
}

BeatSaver& BeatSaver::sharedInstance() {
    static BeatSaver *_instance = nullptr;
    if (!_instance) {
        _instance = new BeatSaver();
    }
    return *_instance;
}

void BeatSaver::retrieveSongByKey(const BeatSaverKey &key, CallbackFunc<BeatSaverSong> handler) {
    auto requestUrl = string_format(mapsDetailByKeyUrlFormat, endpoint.data(), key.c_str());
    StartGenericRequestCoro<BeatSaverSong>(requestUrl, std::move(handler));
}

void BeatSaver::retrievePage(std::string path, int page, CallbackFunc<BeatSaverPage> handler) {
    auto requestUrl = string_format(pageUrlFormat, endpoint.c_str(), path.c_str(), page);

    getLogger().info("Retrieving page %d of %s", page, path.data());

    StartGenericRequestCoro<BeatSaverPage>(requestUrl, std::move(handler));
}

void BeatSaver::retrievePageByRating(int page, CallbackFunc<BeatSaverPage> handler) {
    retrievePage("/maps/rating/", page, std::move(handler));
}

void BeatSaver::retrievePageByLatest(int page, CallbackFunc<BeatSaverPage> handler) {
    retrievePage("/maps/latest/", page, std::move(handler));
}

void BeatSaver::retrievePageByDownloads(int page, CallbackFunc<BeatSaverPage> handler) {
    retrievePage("/maps/downloads/", page, std::move(handler));
}

void BeatSaver::retrievePageByPlays(int page, CallbackFunc<BeatSaverPage> handler) {
    retrievePage("/maps/plays/", page, std::move(handler));
}

void BeatSaver::retrievePageByHeat(int page, CallbackFunc<BeatSaverPage> handler) {
    retrievePage("/maps/hot/", page, std::move(handler));
}

void BeatSaver::searchByText(const std::string& query, int page, CallbackFunc<BeatSaverPage> handler) {
    auto encodedQuery = encodeURIComponent(query);
    auto requestUrl = string_format(searchUrlFormat, endpoint.data(), page, encodedQuery.data());

    getLogger().info("Searching \"%s\" page %d...%s", query.c_str(), page, requestUrl.data());

    StartGenericRequestCoro<BeatSaverPage>(requestUrl, std::move(handler));
}

SongDownloadingTask::SongDownloadingTask(std::string key): key(std::move(key)) {
    future = result.get_future().share();
    webRequest = nullptr;
}

void SongDownloadingTask::onDownloadTaskCompletion() {
    auto responseCode = webRequest->get_responseCode();
    auto responseStatusStr = to_utf8(csstrtostr(UnityWebRequest::GetHTTPStatusString(responseCode)));

    getLogger().info("Song downloading task completed with status: %ld %s", responseCode, responseStatusStr.data());

    auto downloadHandler = webRequest->get_downloadHandler();
    auto downloadedData = downloadHandler->GetData();

    auto downloadedSong = DownloadedSong {};

    if (responseCode != 200) {
        std::string errorString;

        switch (responseCode) {
        case 404:
            errorString = "Song not found";
            break;
        case 429:
            errorString = "Rate limited";
            break;
        default: errorString = string_format("Error status code %d %s", responseCode, responseStatusStr.data());
        }

        getLogger().info("Song download failed because of error: %s", errorString.data());
        downloadException.emplace(std::runtime_error(errorString));
    } else if (downloadedData->Length() > 0) {
        // Extract to tmp directory
        std::string extractDirPath = getCustomSongFolder() + "/" + key;

        if (!direxists(extractDirPath.data())) {
            mkpath(extractDirPath.data());
        }

        // Save downloaded archive to tmp path
        auto levelArchivePath = extractDirPath + "/" + key + ".zip";
        getLogger().info("Saving downloaded song archive to %s", levelArchivePath.data());

        std::ofstream archiveWriteStream;
        archiveWriteStream.open(levelArchivePath, std::ios::binary | std::ios::out);
        archiveWriteStream.write(reinterpret_cast<const char *>(downloadedData->values), downloadedData->Length());
        archiveWriteStream.close();

        // Extract archive
        getLogger().info("Extracting archive %s", levelArchivePath.data());

        int args = 2;
        zip_extract(levelArchivePath.data(), extractDirPath.data(), +[](const char *name, void *arg) -> int {
            getLogger().info("Extracted file: %s", name);
            return 0;
        }, &args);

        // Delete archive file
        std::remove(levelArchivePath.data());

        downloadedSong.levelPath = extractDirPath;
        downloadedSong.key = key;
    } else {
        getLogger().error("Unable to download song: zero-length response");
        downloadException.emplace(std::runtime_error("Zero-length response"));
    }

    // Call completion handlers
    getLogger().info("Trying to lock...");
    handlersMutex.lock();
    if (downloadException.has_value()) {
        result.set_exception(std::make_exception_ptr(*downloadException));
    } else {
        result.set_value(downloadedSong);
    }
    getLogger().info("done, exiting critical section.");
    handlersMutex.unlock();

    for (const auto& h : completionHandlers) {
        h(downloadedSong, downloadException.has_value() ? -1 : 0);
    }
}

void SongDownloadingTask::onCompletion(std::function<void(const DownloadedSong&, const DownloadError)> handler) {
    try {
        // Call the handler directly if there's already a valid state
        getLogger().info("Locking handlers mutex from onCompletion");
        handlersMutex.lock();
        if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            getLogger().info("Song future state valid. Returning directly...");
            handlersMutex.unlock(); // Unlock before we have a chance to throw
            handler(future.get(), 0);
            return;
        }
        completionHandlers.push_back(std::move(handler));
        getLogger().info("Added handler");
        handlersMutex.unlock();
    } catch (...) {
        handler({}, -1);
    }
}

std::string SongDownloadingTask::getCustomSongFolder() {
    auto allMods = Modloader::getMods();
    auto songLoaderModItr = allMods.find("SongLoader");

    if (songLoaderModItr == allMods.end()) {
        getLogger().error("SongLoader not found!! Crashing the game...");
        SAFE_ABORT();
    }

    auto songLoaderMod = *songLoaderModItr;
    auto dataDir = getDataDir(songLoaderMod.second.info) + "CustomLevels";

    if (!direxists(dataDir.data())) {
        mkpath(dataDir.data());
    }

    return dataDir;
}

BeatSaver::BeatSaver() {
    endpoint = beatsaverDefaultEndpoint;
    userAgent = il2cpp_utils::createcsstr(
        "QuestMoreSongs/" + getModInfo().version,
        il2cpp_utils::StringType::Manual
        );
}

#pragma clang diagnostic pop
