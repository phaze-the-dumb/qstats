#include "main.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "HMUI/CurvedTextMeshPro.hpp"
#include "UnityEngine/GameObject.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "HMUI/Touchable.hpp"
#include "questui/shared/QuestUI.hpp"
#include "config-utils/shared/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "ModConfig.hpp"
#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "System/Action.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"

using namespace QuestUI;
using namespace UnityEngine;
using namespace GlobalNamespace;

static ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup
DEFINE_CONFIG(ModConfig);

TMPro::TextMeshProUGUI* notesHitText;
TMPro::TextMeshProUGUI* notesMissedText;
TMPro::TextMeshProUGUI* badCutsText;
TMPro::TextMeshProUGUI* levelsPlayedText;
TMPro::TextMeshProUGUI* totalScoreText;
TMPro::TextMeshProUGUI* totalBlocksText;

// Loads the config from disk using our modInfo, then returns it for use
Configuration& getConfig() {
    static Configuration config(modInfo);
    config.Load();
    return config;
}

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getConfig().Load(); // Load the config file
    getLogger().info("Completed setup!");
}

MAKE_HOOK_MATCH(PlatformLeaderboardViewController_DidActivate, &PlatformLeaderboardViewController::DidActivate, void,
    PlatformLeaderboardViewController* self,
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    PlatformLeaderboardViewController_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);

    if(firstActivation){
        UI::HorizontalLayoutGroup* layout = BeatSaberUI::CreateHorizontalLayoutGroup(self->get_transform());
        RectTransform* rectTransform = layout->get_gameObject()->GetComponent<UnityEngine::RectTransform*>();

        rectTransform->set_anchoredPosition(UnityEngine::Vector2(0.0f, 30.0f));
        rectTransform->set_localScale({0.75, 0.75, 1});

        HMUI::ModalView* modal = BeatSaberUI::CreateModal(self->get_transform(), UnityEngine::Vector2(60.0f, 40.0f), [](HMUI::ModalView* modal){}, true);
        UnityEngine::GameObject* container = BeatSaberUI::CreateScrollableModalContainer(modal);
        
        notesHitText = BeatSaberUI::CreateText(container->get_transform(), "Notes Hit: " + std::to_string(getModConfig().BlocksHit.GetValue()));
        notesMissedText = BeatSaberUI::CreateText(container->get_transform(), "Notes Missed: " + std::to_string(getModConfig().BlocksMissed.GetValue()));
        badCutsText = BeatSaberUI::CreateText(container->get_transform(), "Bad Cuts: " + std::to_string(getModConfig().BadCuts.GetValue()));
        levelsPlayedText = BeatSaberUI::CreateText(container->get_transform(), "Levels Played: " + std::to_string(getModConfig().LevelsPlayed.GetValue()));
        totalScoreText = BeatSaberUI::CreateText(container->get_transform(), "Total Score: " + std::to_string(getModConfig().TotalScore.GetValue()));
        totalBlocksText = BeatSaberUI::CreateText(container->get_transform(), "Total Blocks: " + std::to_string(getModConfig().BlocksHit.GetValue() + getModConfig().BlocksMissed.GetValue() + getModConfig().BadCuts.GetValue()));

        BeatSaberUI::CreateUIButton(rectTransform->get_transform(), "User Stats",
            [modal]() {
                getLogger().info("Button Clicked");
                
                modal->Show(true, true, il2cpp_utils::MakeDelegate<System::Action*>(classof(System::Action*),
                    (std::function<void()>) []() {
                        getLogger().info("Modal Loaded");
                    }
                ));
            });
    } else{
        notesHitText->set_text(il2cpp_utils::newcsstr("Notes Hit: " + std::to_string(getModConfig().BlocksHit.GetValue())));
        notesMissedText->set_text(il2cpp_utils::newcsstr("Notes Missed: " + std::to_string(getModConfig().BlocksMissed.GetValue())));
        badCutsText->set_text(il2cpp_utils::newcsstr("Bad Cuts: " + std::to_string(getModConfig().BadCuts.GetValue())));
        levelsPlayedText->set_text(il2cpp_utils::newcsstr("Levels Played: " + std::to_string(getModConfig().LevelsPlayed.GetValue())));
        totalScoreText->set_text(il2cpp_utils::newcsstr("Total Score: " + std::to_string(getModConfig().TotalScore.GetValue())));
        totalBlocksText->set_text(il2cpp_utils::newcsstr("Total Blocks: " + std::to_string(getModConfig().BlocksHit.GetValue() + getModConfig().BlocksMissed.GetValue() + getModConfig().BadCuts.GetValue())));
    }
}

MAKE_HOOK_MATCH(ResultsViewController_Init, &ResultsViewController::Init, void,
    ResultsViewController* self,
    LevelCompletionResults* result,
    IDifficultyBeatmap* beatmap,
    bool practice,
    bool newHighScore
) {
    ResultsViewController_Init(self, result, beatmap, practice, newHighScore);

    getModConfig().LevelsPlayed.SetValue(getModConfig().LevelsPlayed.GetValue() + 1);
    getLogger().info("Level Finished");

    if(!practice){
        getModConfig().TotalScore.SetValue(getModConfig().TotalScore.GetValue() + result->rawScore);
        getLogger().info("Score added: %d", result->rawScore);
    };
}

MAKE_HOOK_MATCH(ScoreController_HandleNoteWasCut, &ScoreController::HandleNoteWasCut, void,
    ScoreController* self,
    NoteController* note,
    ByRef<GlobalNamespace::NoteCutInfo> info
) {
    ScoreController_HandleNoteWasCut(self, note, info);

    if(info.heldRef.get_allIsOK()){
        getModConfig().BlocksHit.SetValue(getModConfig().BlocksHit.GetValue() + 1);

        getLogger().info("Block Hit");
    } else{
        getModConfig().BadCuts.SetValue(getModConfig().BadCuts.GetValue() + 1);

        getLogger().info("Bad Cut");
    };
}

MAKE_HOOK_MATCH(ScoreController_HandleNoteWasMissed, &ScoreController::HandleNoteWasMissed, void,
    ScoreController* self,
    NoteController* note
) {
    ScoreController_HandleNoteWasMissed(self, note);

    getModConfig().BlocksMissed.SetValue(getModConfig().BlocksMissed.GetValue() + 1);

    getLogger().info("Block Missed");
}

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    if(firstActivation){
        self->get_gameObject()->AddComponent<HMUI::Touchable*>();
        UnityEngine::GameObject* settings = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
    
        notesHitText = BeatSaberUI::CreateText(settings->get_transform(), "Notes Hit: " + std::to_string(getModConfig().BlocksHit.GetValue()));
        notesMissedText = BeatSaberUI::CreateText(settings->get_transform(), "Notes Missed: " + std::to_string(getModConfig().BlocksMissed.GetValue()));
        badCutsText = BeatSaberUI::CreateText(settings->get_transform(), "Bad Cuts: " + std::to_string(getModConfig().BadCuts.GetValue()));
        levelsPlayedText = BeatSaberUI::CreateText(settings->get_transform(), "Levels Played: " + std::to_string(getModConfig().LevelsPlayed.GetValue()));
        totalScoreText = BeatSaberUI::CreateText(settings->get_transform(), "Total Score: " + std::to_string(getModConfig().TotalScore.GetValue()));
    } else{
        notesHitText->set_text(il2cpp_utils::newcsstr("Notes Hit: " + std::to_string(getModConfig().BlocksHit.GetValue())));
        notesMissedText->set_text(il2cpp_utils::newcsstr("Notes Missed: " + std::to_string(getModConfig().BlocksMissed.GetValue())));
        badCutsText->set_text(il2cpp_utils::newcsstr("Bad Cuts: " + std::to_string(getModConfig().BadCuts.GetValue())));
        levelsPlayedText->set_text(il2cpp_utils::newcsstr("Levels Played: " + std::to_string(getModConfig().LevelsPlayed.GetValue())));
        totalScoreText->set_text(il2cpp_utils::newcsstr("Total Score: " + std::to_string(getModConfig().TotalScore.GetValue())));
    };
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();
    getModConfig().Init(modInfo);

    LoggerContextObject logger = getLogger().WithContext("load");

    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);
    getLogger().info("Successfully installed Settings UI!");

    getLogger().info("Installing hooks...");
    //INSTALL_HOOK(logger, MainMenuViewController_DidActivate);
    INSTALL_HOOK(logger, PlatformLeaderboardViewController_DidActivate);
    INSTALL_HOOK(logger, ResultsViewController_Init);
    INSTALL_HOOK(logger, ScoreController_HandleNoteWasCut);
    INSTALL_HOOK(logger, ScoreController_HandleNoteWasMissed);
    getLogger().info("Installed all hooks!");
}