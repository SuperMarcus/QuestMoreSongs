#pragma once

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <future>

namespace UnityEngine::Networking {
    class UnityWebRequest;
}

namespace moresongs {
    // Forward declarations
    class BeatSaver;
    class SongDownloadingTask;
    struct DownloadedSong;

    using SongDownloadingTaskPtr = std::shared_ptr<SongDownloadingTask>;
    using BeatSaverKey = std::string;

    class BeatSaver {
    public:
        SongDownloadingTaskPtr downloadSongWithKey(const BeatSaverKey& key);
        static BeatSaver& sharedInstance();

    private:
        std::unordered_map<BeatSaverKey, SongDownloadingTaskPtr> downloadTasks;
    };

    struct DownloadedSong {
    public:
        std::string levelPath;
        std::string key;
    };

    class SongDownloadingTask {
    public:
        using DownloadError = uint8_t;

        std::string key;
        std::optional<std::exception> downloadException;

        explicit SongDownloadingTask(std::string key);

        void onCompletion(std::function<void(const DownloadedSong&, const DownloadError)> handler);

        static std::string getCustomSongFolder();

    private:
        std::vector<std::function<void(const DownloadedSong&, const DownloadError)>> completionHandlers;
        std::promise<DownloadedSong> result;
        std::shared_future<DownloadedSong> future;
        std::mutex handlersMutex {};

    private:
        friend class BeatSaver;
        class UnityEngine::Networking::UnityWebRequest* webRequest;

        void onDownloadTaskCompletion();
    };
}
