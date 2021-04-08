#pragma once

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <future>

#include "extern/beatsaber-hook/shared/config/rapidjson-utils.hpp"

namespace UnityEngine::Networking {
    class UnityWebRequest;
}

namespace moresongs {
    // Forward declarations
    class BeatSaver;
    class SongDownloadingTask;
    struct DownloadedSong;
    struct BeatSaverSong;
    struct BeatSaverPage;

    using SongDownloadingTaskPtr = std::shared_ptr<SongDownloadingTask>;
    using BeatSaverKey = std::string;
    using JsonObject = decltype(rapidjson::Document().GetObject());
    using ConstJsonObject = decltype(static_cast<const rapidjson::Document>(rapidjson::Document()).GetObject());

    template <typename ResultType>
    using CallbackFunc = std::function<void(std::optional<ResultType>)>;

    class BeatSaver {
    public:
        SongDownloadingTaskPtr downloadSongWithKey(const BeatSaverKey& key);

        void retrieveSongByKey(const BeatSaverKey& key, CallbackFunc<BeatSaverSong> handler);

        void retrievePageByRating(int page, CallbackFunc<BeatSaverPage> handler);
        void retrievePageByLatest(int page, CallbackFunc<BeatSaverPage> handler);
        void retrievePageByDownloads(int page, CallbackFunc<BeatSaverPage> handler);
        void retrievePageByPlays(int page, CallbackFunc<BeatSaverPage> handler);
        void retrievePageByHeat(int page, CallbackFunc<BeatSaverPage> handler);

        void searchByText(const std::string& query, int page, CallbackFunc<BeatSaverPage> handler);

        static BeatSaver& sharedInstance();

    private:
        std::unordered_map<BeatSaverKey, SongDownloadingTaskPtr> downloadTasks;
        std::string endpoint;

        BeatSaver();
        void retrievePage(std::string path, int page, CallbackFunc<BeatSaverPage> handler);
    };

    /// Available difficulties
    struct BeatSaverSongDifficulties {
        bool easy;
        bool normal;
        bool hard;
        bool expert;
        bool expertPlus;

        explicit BeatSaverSongDifficulties(const ConstJsonObject &);
    };

    /// Characteristic difficulty info
    struct BeatSaverSongCharacteristicDifficultyData {
        float duration;
        int length;
        int bombs;
        int notes;
        int obstacles;
        int njs;
        int njsOffset;

        explicit BeatSaverSongCharacteristicDifficultyData(const ConstJsonObject &);
    };

    /// Characteristic difficulties
    struct BeatSaverSongCharacteristicDifficulties {
        std::optional<BeatSaverSongCharacteristicDifficultyData> easy;
        std::optional<BeatSaverSongCharacteristicDifficultyData> normal;
        std::optional<BeatSaverSongCharacteristicDifficultyData> hard;
        std::optional<BeatSaverSongCharacteristicDifficultyData> expert;
        std::optional<BeatSaverSongCharacteristicDifficultyData> expertPlus;

        explicit BeatSaverSongCharacteristicDifficulties(const ConstJsonObject &);
    };

    /// Characteristic variant of a song
    struct BeatSaverSongCharacteristic {
        std::string name;
        BeatSaverSongCharacteristicDifficulties difficulties;

        explicit BeatSaverSongCharacteristic(const ConstJsonObject &);
    };

    /// Metadata fields of a song
    struct BeatSaverSongMetadata {
        BeatSaverSongDifficulties difficulties;
        std::vector<BeatSaverSongCharacteristic> characteristics;
        int duration;
        int bpm;
        std::string levelAuthorName;
        std::string songAuthorName;
        std::string songName;
        std::string songSubName;

        explicit BeatSaverSongMetadata(const ConstJsonObject &);
    };

    /// Song information
    struct BeatSaverSong {
        BeatSaverKey key;
        BeatSaverSongMetadata metadata;
        std::string name;
        std::string description;
        std::string hash;
        std::string coverURL;

        explicit BeatSaverSong(const ConstJsonObject &);
    };

    /// A page of BeatSaverSongs
    struct BeatSaverPage {
        std::vector<BeatSaverSong> docs;
        int totalDocs;
        int lastPage;
        std::optional<int> prevPage;
        std::optional<int> nextPage;

        explicit BeatSaverPage(const ConstJsonObject &);

    private:
        std::string pageRequestEndpoint;
    };

    /// A downloaded song from SongDownloadngTask
    struct DownloadedSong {
    public:
        std::string levelPath;
        std::string key;
    };

    /// Song downloading task
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
