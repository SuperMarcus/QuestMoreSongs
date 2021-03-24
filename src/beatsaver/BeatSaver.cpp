#include "utils/utils.h"
#include "utils/il2cpp-utils.hpp"

#include "UnityEngine/Networking/UnityWebRequest.hpp"
#include "UnityEngine/Networking/DownloadHandler.hpp"
#include "UnityEngine/Networking/UnityWebRequestAsyncOperation.hpp"
#include "UnityEngine/AsyncOperation.hpp"
#include "System/Action_1.hpp"

#include "songloader/include/Utils/HashUtils.hpp"
#include "songloader/include/CustomTypes/SongLoader.hpp"

#include "BeatSaver.h"
#include "moresongs_private.h"
#include "zip.h"

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

constexpr auto downloadUrlFormat = "https://beatsaver.com/api/download/key/%s"sv;

moresongs::SongDownloadingTaskPtr moresongs::BeatSaver::downloadSongWithKey(const moresongs::BeatSaverKey& key) {
    if (downloadTasks.contains(key)) {
        return downloadTasks[key];
    }

    auto downloadTask = std::make_shared<SongDownloadingTask>(key);
    downloadTasks[key] = downloadTask;

    // Begin downloading the song
    auto downloadUrl = string_format(downloadUrlFormat, key.c_str());
    downloadTask->webRequest = UnityEngine::Networking::UnityWebRequest::Get(il2cpp_utils::createcsstr(downloadUrl));

    // Request parameters
    auto userAgent = "QuestMoreSongs/" + getModInfo().version;
    downloadTask->webRequest->SetRequestHeader("User-Agent"_cs, il2cpp_utils::createcsstr(userAgent));
//    downloadTask->webRequest->redirectLimit;

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

    if (downloadedData->Length() > 0) {
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
        downloadException = std::runtime_error("Zero-length response");
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
