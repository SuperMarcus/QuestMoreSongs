#include "utils/il2cpp-utils.hpp"
#include "utils/utils.h"
#include "moresongs_private.h"

#include "UnityEngine/RectOffset.hpp"

#include "QuestUI.hpp"
#include "BeatSaberUI.hpp"
#include "CustomTypes/Components/Backgroundable.hpp"
#include "CustomTypes/MoreSongsViewController.h"

#include "songloader/include/CustomTypes/SongLoader.hpp"

#include "BeatSaver.h"

using namespace moresongs;
using namespace moresongs::literals;
using namespace moresongs::CustomTypes;

DEFINE_CLASS(MoreSongsViewController);

void MoreSongsViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if (firstActivation && addedToHierarchy) {
        getLogger().info("Adding UI elements...");

        auto sectionBackgroundName = "round-rect-panel"_cs;
        auto sectionPadding = UnityEngine::RectOffset::New_ctor(4, 4, 4, 4);

        // Root Layout
        auto rootLayout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(get_rectTransform());
        rootLayout->set_spacing(4);

        // Layout - Section Container - H
        auto sectionContainerLayout = QuestUI::BeatSaberUI::CreateHorizontalLayoutGroup(rootLayout->get_rectTransform());
        sectionContainerLayout->set_spacing(4);

        // Layout - Center Container - V
        auto centerContainerLayout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(sectionContainerLayout->get_rectTransform());
        centerContainerLayout->set_spacing(4);

        auto centerSectionLayout = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(centerContainerLayout->get_rectTransform());
        centerSectionLayout->get_gameObject()
            ->AddComponent<QuestUI::Backgroundable*>()
            ->ApplyBackground(sectionBackgroundName);

        auto statusText = QuestUI::BeatSaberUI::CreateText(centerSectionLayout->get_transform(), "Idle");

        auto keyInputBox = QuestUI::BeatSaberUI::CreateStringSetting(
            centerSectionLayout->get_transform(),
            "Key", "",
            std::bind(&MoreSongsViewController::onSearchInputUpdate, this, std::placeholders::_1)
        );
        QuestUI::BeatSaberUI::CreateUIButton(
            centerSectionLayout->get_transform(),
            "Download", [=]() {
                auto keyString = to_utf8(csstrtostr(keyInputBox->get_text()));
                getLogger().info("Looking for song with key %s", keyString.data());

                statusText->set_text("Downloading song..."_cs);

                auto task = BeatSaver::sharedInstance().downloadSongWithKey(keyString);
                task->onCompletion([statusText](const DownloadedSong& song, const SongDownloadingTask::DownloadError error) {
                    auto newStatusText = string_format("Downloaded to %s, error: %d", song.levelPath.data(), error);
                    statusText->set_text(il2cpp_utils::createcsstr(newStatusText));
                    RuntimeSongLoader::SongLoader::GetInstance()->RefreshSongs(true);
                });
            });
    }
}

void MoreSongsViewController::onSearchInputUpdate(std::string data) {
    // TODO
}

void MoreSongsViewController::DidDeactivate(bool removedFromHierarchy, bool screenSystemDisabling) {
    getLogger().info("Deactivating view controller");
}
