
//Include statements
#include <windows.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ShellAPI.h>
#include <filesystem>
#include <format>
#include <string>

#include "../functions/functions.hpp"              // Miscellaneous custom functions
#include "../functions/vrinput.hpp"                // VR controller input capture functionality
#include "../functions/websocket.hpp"              // Websocket functionality
#include "../events/animation-events.hpp"          // Animation event hooking and processing
#include "../events/spell-learned-event.hpp"       // Spell learn event hooking and processing
#include "../events/morph-changed-event.hpp"       // Player morph event hooking and processing
#include "../events/load-game-event.hpp"           // Game load event hooking and processing
#include "../events/menu-close-event.hpp"          // Menu close event hooking and processing
#include "../events/location-discovery-event.hpp"  // Location discovery event hooking and processing
#include "../events/device-input-event.hpp"        // Flatrim device input event hooking and processing
#include "../functions/logger.hpp"                 // SKSE log functions

struct Command
{
    std::string Name = "";
    RE::FormID ID = 0;
    int KeybindDuration = 0;
    std::string Type = "";
    std::string fileName = "";
    int Hand = 0;
    int AutoCast = 0;
    std::string Morph = "";
    int CommandNum = 0;
};

#pragma region Function Definitions
void InitialUpdate();
void CheckUpdate(bool loop = false, bool isAsync = false);
void Update(std::string update = "");
void ExecuteCommand(Command command);
#pragma endregion

#pragma region Global Variables
std::string id = "";
int currentMorph = Morph::Player;
bool currentVocalPTS;
float currentSensitivity;
float currentAutoCastShouts;
float currentAutoCastPowers;
int numMagic[] = {0, 0};
std::string knownShouts = "";
std::string currentShouts = "";
std::string knownCommand = "";
std::vector<std::string> knownLocations;
std::vector<std::string> currentLocations;
bool saveTriggered = false;
bool readyForUpdate = true;
bool isRecognitionEnabled = true;
int updateQueue = 0;
#pragma endregion

// Method executed when SKSE messages are received
void OnMessage(SKSE::MessagingInterface::Message* message)
{
    try {
        /* // Debug
        switch (message->type) {
            // Descriptions are taken from the original skse64 library
            // See:
            // https://github.com/ianpatt/skse64/blob/09f520a2433747f33ae7d7c15b1164ca198932c3/skse64/PluginAPI.h#L193-L212
            case SKSE::MessagingInterface::kPostLoad:
                logger::info("kPostLoad: sent to registered plugins once all plugins have been loaded");
                break;
            case SKSE::MessagingInterface::kPostPostLoad:
                logger::info(
                    "kPostPostLoad: sent right after kPostLoad to facilitate the correct dispatching/registering of "
                    "messages/listeners");
                break;
            case SKSE::MessagingInterface::kPreLoadGame:
                // message->dataLen: length of file path, data: char* file path of .ess savegame file
                logger::info("kPreLoadGame: sent immediately before savegame is read");
                break;
            case SKSE::MessagingInterface::kPostLoadGame:
                // You will probably want to handle this event if your plugin uses a Preload callback
                // as there is a chance that after that callback is invoked the game will encounter an error
                // while loading the saved game (eg. corrupted save) which may require you to reset some of your
                // plugin state.
                logger::info("kPostLoadGame: sent after an attempt to load a saved game has finished");
                break;
            case SKSE::MessagingInterface::kSaveGame:
                logger::info("kSaveGame");
                break;
            case SKSE::MessagingInterface::kDeleteGame:
                // message->dataLen: length of file path, data: char* file path of .ess savegame file
                logger::info("kDeleteGame: sent right before deleting the .skse cosave and the .ess save");
                break;
            case SKSE::MessagingInterface::kInputLoaded:
                logger::info("kInputLoaded: sent right after game input is loaded, right before the main menu initializes");
                break;
            case SKSE::MessagingInterface::kNewGame:
                // message-data: CharGen TESQuest pointer (Note: I haven't confirmed the usefulness of this yet!)
                logger::info("kNewGame: sent after a new game is created, before the game has loaded");
                break;
            case SKSE::MessagingInterface::kDataLoaded:
                logger::info("kDataLoaded: sent after the data handler has loaded all its forms");
                break;
            default:
                logger::info("Unknown system message of type: {}", message->type);
                break;
        } */

        switch (message->type) {
            // All plugins have been loaded
            case SKSE::MessagingInterface::kPostLoad:
                ConfigureWebsocketPort();  // Write target websocket port to file, which will be read by speech recognition application
                LaunchSpeechRecoApp();     // Launch the companion speech recognition application
                /// MessageBoxA(NULL, "Attach Voxima plugin debugger to Skyrim game process now. Press OK when you're ready!", "Skyrim Voxima (C++)", MB_OK | MB_ICONQUESTION);  //
                /// MessageBox to halt execution so a debugger can be attached
                if (REL::Module::IsVR()) {
                    logger::debug("SKSE PostLoad message received, registering for PapyrusVR messages from SkyrimVRTools");
                    SKSE::GetMessagingInterface()->RegisterListener("SkyrimVRTools", OnPapyrusVRMessage);
                }
                break;

            // Data handler has loaded all its forms (Main menu has loaded???)
            case SKSE::MessagingInterface::kDataLoaded:
                InitializeWebSocketClient();           // Initialize the websocket client owned by the plugin
                InitializeLoadGameHooking();           // Setup game load event monitoring
                InitializeAnimationHooking();          // Setup animation event monitoring
                InitializeSpellLearnHooking();         // Setup spell learn event monitoring
                InitializeMorphChangeHooking();        // Setup player morph change event monitoring
                InitializeMenuOpenCloseHooking();      // Setup menu open/close event monitoring
                InitializeLocationDiscoveryHooking();  // Setup location discovery event monitoring
                if (REL::Module::IsVR()) {
                    if (g_papyrusvr) {
                        OpenVRHookManagerAPI* hookMgrAPI = RequestOpenVRHookManagerObject();
                        hookMgrAPI = NULL;
                        if (hookMgrAPI) {
                            logger::debug("Using RAW OpenVR Hook API.");
                            g_VRSystem = hookMgrAPI->GetVRSystem();  // setup VR system before callbacks
                            hookMgrAPI->RegisterControllerStateCB(OnControllerStateChanged);
                            // hookMgrAPI->RegisterGetPosesCB(OnGetPosesUpdate);
                        }
                        else {
                            logger::debug("Using legacy PapyrusVR API.");

                            // Registers for PoseUpdates
                            g_papyrusvr->GetVRManager()->RegisterVRButtonListener(OnVRButtonEvent);
                            // g_papyrusvr->GetVRManager()->RegisterVRUpdateListener(OnVRUpdateEvent);
                        }
                    }
                    else {
                        logger::debug("PapyrusVR was not initialized!");
                    }
                }
                else
                    InitializeFlatrimDeviceInputHooking();                        // Setup "Flatrim" (non-VR Skyrim) device input event monitoring
                while (connected == false)                                        // Loop while websocket connection has not been made
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Brief pause to allow for websocket connection to be made
                InitialUpdate();                                                  // Run initial data update
                break;

            //// Save game has been loaded
            // case SKSE::MessagingInterface::kPostLoadGame:
            //     logger::debug("Saved game loaded!!");
            //     CheckUpdate();
            //     break;

            // Save Game as been created (manual or autosave)
            case SKSE::MessagingInterface::kSaveGame:
                if (saveTriggered == false) {
                    saveTriggered = true;
                    logger::debug("Save game created!!");
                    CheckUpdate();
                }
                break;
        }  // End switch
    }
    catch (const std::exception& ex) {
        logger::error("ERROR during OnMessage: {}", ex.what());
    }
}

// Initial game data gathering
void InitialUpdate()
{
    try {
        // logger::info("{}", "Path is " + SKSE::log::log_directory().value().string());
        // SendMessage("Path is " + SKSE::log::log_directory().value().string());

        // Start Mod
        logger::info("Starting Mod");

        player = RE::PlayerCharacter::GetSingleton();

        VOX_Enabled = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_Enabled");
        VOX_UpdateInterval = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_UpdateInterval");
        VOX_CheckForUpdate = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_CheckForUpdate");
        VOX_PushToSpeakType = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_PushToSpeakType");
        VOX_PushToSpeakKeyCode = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_PushToSpeakKeyCode");
        VOX_ShoutKey = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_ShoutKey");
        VOX_ShowLog = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_ShowLog");
        VOX_LongAutoCast = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_LongAutoCast");
        VOX_AutoCastPowers = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_AutoCastPowers");
        VOX_AutoCastShouts = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_AutoCastShouts");
        VOX_KnownShoutWordsOnly = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_KnownShoutWordsOnly");

        // C++ --> C# Variables
        // VOX_VocalPushToSpeak = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_VocalPushToSpeak");
        VOX_Sensitivity = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_Sensitivity");
        currentSensitivity = VOX_Sensitivity->value;
        currentAutoCastShouts = VOX_AutoCastShouts->value;
        currentAutoCastPowers = VOX_AutoCastPowers->value;
    }
    catch (const std::exception& ex) {
        logger::error("ERROR during InitialUpdate: {}", ex.what());
    }
}

// Checks for changes in the tracked data
void CheckUpdate(bool loop, bool isAsync)
{
    try {
        // If the function isn't async, run it in a different thread
        if (isAsync == false) {
            std::jthread update(CheckUpdate, loop, true);
            update.detach();
            return;
        }

        logger::debug("CheckUpdate method running");
        bool fileUpdate;
        bool fullUpdate;
        int placeInQueue = updateQueue;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (placeInQueue < updateQueue) return;

        updateQueue++;

        if (loop) logger::debug("CheckUpdate loop start");

        do {
            if (!player->Is3DLoaded()) {
                if (loop) std::this_thread::sleep_for(std::chrono::seconds(1));  // 1 second delay to avoid looping unnecessarily
                continue;
            }
            else if (VOX_Enabled->value == 0) {
                if (loop)
                    std::this_thread::sleep_for(std::chrono::seconds(5));  // 5 second delay to not loop unnecessarily. See Bottom of page Note (1)
                else {
                    SendMessage("disable recognition");
                    isRecognitionEnabled = false;
                }

                continue;
            }
            else if (VOX_Enabled->value == 1 && !isRecognitionEnabled) {
                SendMessage("enable recognition");
                isRecognitionEnabled = true;
            }
            // Have the C# program recieve the above messages and enable/disable recognition accordingly*****

            // VOX_PushToSpeak

            fileUpdate = false;
            fullUpdate = false;

#pragma region Check for Update

            // Checks for change in number of magic items (includes effects)
            if (numMagic[0] != player->GetActorRuntimeData().addedSpells.size()) {
                numMagic[0] = player->GetActorRuntimeData().addedSpells.size();

                // Checks for a change in the number Spells or Powers
                int currentSpells = 0;
                for (RE::SpellItem* item : player->GetActorRuntimeData().addedSpells) {
                    switch (item->GetSpellType()) {
                        case RE::MagicSystem::SpellType::kSpell:
                        case RE::MagicSystem::SpellType::kPower:
                        case RE::MagicSystem::SpellType::kLesserPower:
                        case RE::MagicSystem::SpellType::kVoicePower:
                            currentSpells++;
                            break;
                    }  // End Switch
                }

                // If there is a change, update the recorded total and mark for Update()
                if (currentSpells != numMagic[1]) {
                    numMagic[1] = currentSpells;
                    fullUpdate = true;
                }
            }

            //-----Current Morph-----//

            // Werewolf
            if (IsPlayerWerewolf()) {  // If Werewolf
                if (currentMorph != Morph::Werewolf) {
                    currentMorph = Morph::Werewolf;
                    logger::info("Sending \"Werewolf\" Morph to C#");
                    std::this_thread::sleep_for(std::chrono::seconds(1));  // Delay update one second to allow everything to update. This prevents stacking updates on C#
                    fileUpdate = true;
                }
                currentMorph = Morph::Werewolf;  ///*** is this needed?

                // Vampire Lord
            }
            else if (IsPlayerVampireLord()) {  // If Vampire Lord
                if (currentMorph != Morph::VampireLord) {
                    currentMorph = Morph::VampireLord;
                    logger::info("Sending \"Vampire Lord\" Morph to C#");
                    std::this_thread::sleep_for(std::chrono::seconds(1));  // Delay update one second to allow everything to update. This prevents stacking updates on C#
                    fullUpdate = true;
                }
                currentMorph = Morph::VampireLord;  ///*** is this needed?

                // "Normal" player morph
            }
            else if (currentMorph != Morph::Player) {  // If not a morph but marked as one
                currentMorph = Morph::Player;
                logger::info("Sending \"Reverting Morph\" to C#");
                std::this_thread::sleep_for(std::chrono::seconds(1));  // Delay update one second to allow everything to update. This prevents stacking updates on C#
                fullUpdate = true;
            }

            //-----Current Mount-----//
            switch (PlayerMount()) {
                case 0:  // None
                    if (currentMount != Mount::None) {
                        currentMount = Mount::None;
                        logger::info("Sending \"None\" riding to C#");
                        fileUpdate = true;
                    }
                    currentMount = Mount::None;  ///*** is this needed?
                    break;
                case 1:  // Horse
                    if (currentMount != Mount::Horse) {
                        currentMount = Mount::Horse;
                        logger::info("Sending \"Horse\" riding to C#");
                        fileUpdate = true;
                    }
                    currentMount = Mount::Horse;  ///*** is this needed?
                    break;
                case 2:  // Dragon
                    if (currentMount != Mount::Dragon) {
                        currentMount = Mount::Dragon;
                        logger::info("Sending \"Dragon\" riding to C#");
                        fileUpdate = true;
                    }
                    currentMount = Mount::Dragon;  ///*** is this needed?
                    break;
            }

            //-----MCM-----//
            if (currentVocalPTS && VOX_PushToSpeakType->value != 3 || !currentVocalPTS && VOX_PushToSpeakType->value == 3) {
                currentVocalPTS = !currentVocalPTS;
                logger::info("Sending \"VOX_VocalPushToSpeak = {}\" to C#", currentVocalPTS);
                fileUpdate = true;
            }

            if (currentSensitivity != VOX_Sensitivity->value) {
                currentSensitivity = VOX_Sensitivity->value;
                logger::info("Sending \"VOX_Sensitivity = {}\" to C#", VOX_Sensitivity->value);
                fileUpdate = true;
            }

            if (currentAutoCastShouts != VOX_AutoCastShouts->value) {
                currentAutoCastShouts = VOX_AutoCastShouts->value;
                logger::info("Sending \"VOX_AutoCastShouts = {}\" to C#", VOX_AutoCastShouts->value);
                fileUpdate = true;
            }

            if (currentAutoCastPowers != VOX_AutoCastPowers->value) {
                currentAutoCastPowers = VOX_AutoCastPowers->value;
                logger::info("Sending \"VOX_AutoCastPowers = {}\" to C#", VOX_AutoCastPowers->value);
                fileUpdate = true;
            }

            if (VOX_CheckForUpdate->value == 1) {
                SendMessage(WebSocketMessage::CheckForMicChange);
                VOX_CheckForUpdate->value = 0;
                logger::info("Forcing an Update Check");
                currentShouts = "";
                fullUpdate = true;
            }

            knownShouts = GetActorMagic(player, MagicType::Shout)[0];  // Obtain currently known shouts

            if (knownShouts != currentShouts) {  // Check if there is an update to the shouts
                currentShouts = knownShouts;
                logger::info("Updating Shouts");
                SendMessage("update shouts\n" + knownShouts);
                fullUpdate = true;
            }  // End Shout Check

            knownLocations = GetKnownLocations();
            if (currentLocations != knownLocations) {
                currentLocations = knownLocations;
                std::string locationsMessage = "";
                std::string currentMarker;

                for (auto playerMapMarker : currentLocations) {
                    locationsMessage += playerMapMarker + "\n";
                }
                logger::info("{}", locationsMessage);

                SendMessage(WebSocketMessage::UpdateLocations + locationsMessage + "playerlocation");
            }

            if (fullUpdate)
                Update();
            else if (fileUpdate)
                Update("file");

#pragma endregion

            if (loop) std::this_thread::sleep_for(std::chrono::seconds((int)VOX_UpdateInterval->value));  // Pause based on VOX MCM settings
        } while (loop);                                                                                   // End While
    }
    catch (const std::exception& ex) {
        logger::error("ERROR during CheckUpdate: {}", ex.what());
        if (loop)
            CheckUpdate(true);  // Restart this method so that the mod can continue working
        else
            CheckUpdate();
    }  // End Try/Catch
}  // End CheckUpdate

// Gets and sends game data to C#
void Update(std::string update)
{
    try {
        logger::info("Updating Information");
        bool finished = false;
        std::string type = update;
        std::string updateFile = "";
        std::string updateSpells = "";
        std::string updatePowers = "";
        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });  // sets type.ToLower()

        if (type == "werewolf")  // Morph
            finished = true;
        else if (type == "file") {
            logger::info("Updating C++ -> C# File (PlayerInfo.txt)");
            finished = true;
        }
        else if (type == "")
            logger::info("Updating Everything");

        // Update PlayerInfo File
        switch (currentMorph) {
            case Morph::Werewolf:
                updateFile = "werewolf\ttrue\n";
                break;
            case Morph::VampireLord:
                updateFile = "vampirelord\ttrue\n";
                break;
            default:
                updateFile = "";
                break;
        }

        switch (currentMount) {
            case Mount::Horse:
                updateFile += "horseriding\ttrue\n";
                break;
            case Mount::Dragon:
                updateFile += "dragonriding\ttrue\n";
                break;
        }

        if (currentVocalPTS) updateFile += "VOX_VocalPushToSpeak\t" + std::to_string(currentVocalPTS) + "\n ";

        if (VOX_AutoCastShouts->value == 1)
            updateFile += "VOX_AutoCastShouts\ttrue\n";
        else
            updateFile += "VOX_AutoCastShouts\tfalse\n";

        if (VOX_AutoCastPowers->value == 1)
            updateFile += "VOX_AutoCastPowers\ttrue\n";
        else
            updateFile += "VOX_AutoCastPowers\tfalse\n";

        updateFile += "VOX_Sensitivity\t" + std::to_string((int)VOX_Sensitivity->value);

        SendMessage(WebSocketMessage::UpdateConfiguration + updateFile);

        if (!finished) {
            auto magic = GetActorMagic(player, MagicType::Spell, MagicType::Power);

            SendMessage(WebSocketMessage::UpdateSpells + magic[0]);
            SendMessage(WebSocketMessage::UpdatePowers + magic[1]);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Brief pause to ensure the following message is sent and processed after the previous messages
        SendMessage(WebSocketMessage::InitializeUpdate);
    }
    catch (const std::exception& ex) {
        logger::error("ERROR during Update:\n{}\n", ex.what());
    }
}

// Process messages received from websocket server
void ProcessReceivedMessage(const string& command)
{
    try {
        // Shorten display of message to fit on a single line, for display purposes
        std::string messagePrint = command;
        std::string oldSubstr = "\n";
        std::string newSubstr = "\\n";

        // Replace all occurrences of oldSubstr with newSubstr
        size_t pos = 0;
        while ((pos = messagePrint.find(oldSubstr, pos)) != std::string::npos) {
            messagePrint.replace(pos, oldSubstr.length(), newSubstr);
            pos += newSubstr.length();
        }

        // if (VOX_PushToSpeakKeyCode->value != -1 && !IsKeyDown(VOX_PushToSpeakKeyCode->value)) {
        //     logger::info("Received message but rejected due to Push-To-Speak button not being held: \"{}\"", messagePrint);
        //     return;
        // }

        logger::info("Received message: \"{}\"", messagePrint);

        Command currentCommand;
        std::string line;
        int i = 0, j = 0;

        // Get new command
        for (line = "", j = 0; i < command.length(); j++) {
            for (; i < command.length() && (command[i] == '\r' || command[i] == '\n'); i++)
                ;

            for (line = ""; i < command.length() && command[i] != '\r' && command[i] != '\n'; i++) {
                line += command[i];
            }

            /// logger::info("{}", line);

            switch (j) {
                case 0:
                    if (line == "") continue;
                    currentCommand.Name = line;
                    break;

                case 1:
                    if (line == "") continue;
                    currentCommand.ID = std::stoi(line);
                    break;

                case 2:
                    if (line == "") continue;
                    currentCommand.KeybindDuration = std::stoi(line);
                    break;

                case 3:
                    if (line == "") continue;
                    currentCommand.Type = line;
                    break;

                case 4:
                    if (line == "") continue;
                    currentCommand.fileName = line;
                    break;

                case 5:
                    if (line == "") continue;
                    currentCommand.Hand = std::stoi(line);
                    break;

                case 6:
                    if (line == "") continue;
                    currentCommand.AutoCast = std::stoi(line);
                    break;

                case 7:
                    if (line == "") continue;
                    currentCommand.Morph = line;
                    break;
            }  // End switch
        }      // End for

        switch (PlayerMorph()) {
            case 0:
                if (currentCommand.Morph != "none") {
                    switch (PlayerMount()) {
                        case 0:  // No mount, meaning the player is either Werewolf or Vampire Lord
                            logger::info("Command Ignored. Mismatched Morph. (Current: none) (Command: {})", currentCommand.Morph);
                            return;

                        case 1:  // Horse
                        case 2:  // Dragons
                            break;
                    }
                }
                break;

            case 1:
                if (currentCommand.Morph != "werewolf") {
                    logger::info("Command Ignored. Mismatched Morph. (Current: werewolf) (Command: {})", currentCommand.Morph);
                    return;
                }
                break;

            case 2:
                if (currentCommand.Morph != "vampirelord") {
                    logger::info("Command Ignored. Mismatched Morph. (Current: vampirelord) (Command: {})", currentCommand.Morph);
                    return;
                }
                break;
        }

        logger::info("Executing Command");

        // std::jthread update(ExecuteCommand, currentCommand, true);
        // update.detach();
        ExecuteCommand(currentCommand);
    }
    catch (exception ex) {
        logger::error("ERROR while preprocessing\n\"{}\"\nmessage from server: \"{}\"", command, ex.what());
    }
}

// Executes a given command
void ExecuteCommand(Command command)
{
    const auto task = SKSE::GetTaskInterface();

    if (task != nullptr) {
        task->AddTask([=]() {
            Command currentCommand = command;  // Passed-in "command" argument cannot be modified, so transfer contents to modifiable currentCommand

            // Spell
            if (currentCommand.Type == "spell") {
                id = std::format("{:X}", currentCommand.ID, nullptr, 16);

                for (; id.length() < 6; id = '0' + id)
                    ;

                currentCommand.ID = std::stoi(std::format("{:X}", RE::TESDataHandler::GetSingleton()->GetModIndex(currentCommand.fileName).value()) + id, nullptr, 16);

                RE::SpellItem* item = RE::TESForm::LookupByID<RE::SpellItem>(currentCommand.ID);

                if (item->GetName() == NULL || item->GetName() == "") {
                    logger::info("Spell is NULL");
                    return;
                }

                RE::SpellItem* spell = item->As<RE::SpellItem>();
                bool allowedChargeTime = spell->data.chargeTime <= 1 || VOX_LongAutoCast->value == 1;
                bool allowedCastingType = item->As<RE::MagicItem>()->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget;
                

                bool validCastingItem = allowedCastingType && allowedChargeTime;

                logger::info("Spell Command Identified: Name: \"{}\"     Hand: \"{}\"", item->GetName(), currentCommand.Hand);

                switch (currentCommand.Hand) {
                    // Left
                    case 0:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Left)) {
                            EquipToActor(player, item, ActorSlot::Left);
                        }
                        break;

                    // Right
                    case 1:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Right)) {
                            EquipToActor(player, item, ActorSlot::Right);
                        }
                        break;

                    // Both
                    case 2:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Both)) {
                            EquipToActor(player, item, ActorSlot::Both);
                        }
                        break;

                    // Left with Perk
                    case 3:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Left, 1)) {
                            EquipToActor(player, item, ActorSlot::Left);
                        }
                        break;

                    // Right with Perk
                    case 4:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Right, 1)) {
                            EquipToActor(player, item, ActorSlot::Right);
                        }
                        break;
                }  // End Switch

                // Clear Hands
            }

            // Power
            else if (currentCommand.Type == "power") {
                id = std::format("{:X}", currentCommand.ID, nullptr, 16);

                for (; id.length() < 6; id = '0' + id)
                    ;

                currentCommand.ID = std::stoi(std::format("{:X}", RE::TESDataHandler::GetSingleton()->GetModIndex(currentCommand.fileName).value()) + id, nullptr, 16);

                RE::SpellItem* item = RE::TESForm::LookupByID<RE::SpellItem>(currentCommand.ID);

                if (item->GetName() == NULL || item->GetName() == "") {
                    logger::info("Power is NULL");
                    return;
                }

                logger::info("Power Command Identified: Name: \"{}\"", item->GetName());

                if (currentCommand.AutoCast)
                    CastMagic(player, item, ActorSlot::Voice);
                else
                    EquipToActor(player, item, ActorSlot::Voice);
            }

            // Shout
            else if (currentCommand.Type == "shout") {
                id = std::format("{:X}", currentCommand.ID, nullptr, 16);

                for (; id.length() < 6; id = '0' + id)
                    ;

                currentCommand.ID = std::stoi(std::format("{:X}", RE::TESDataHandler::GetSingleton()->GetModIndex(currentCommand.fileName).value()) + id, nullptr, 16);

                RE::TESShout* item = RE::TESForm::LookupByID<RE::TESShout>(currentCommand.ID);

                if (item->GetName() == NULL || item->GetName() == "") {
                    logger::info("Shout is NULL");
                    return;
                }

                logger::info("Shout Command Identified: Name: \"{}\"     Level: \"{}\"", item->GetName(),
                             currentCommand.Hand + 1);  // Hand meanings: [0] == Level 1   [1] == Level 2   [2] = Level 3

                if (currentCommand.AutoCast)
                    CastMagic(player, item, ActorSlot::Voice, currentCommand.Hand);
                else
                    EquipToActor(player, item, ActorSlot::Voice);

                // Keybind
            }

            // Setting
            else if (currentCommand.Type == "setting") {
#pragma region Clear Hands /Voice
                if (currentCommand.Name == "clear hands") {
                    logger::info("Clear Hands");
                    UnEquipFromActor(player, ActorSlot::Both);
                    SendNotification("Cleared Hands");
                }
                else if (currentCommand.Name == "clear shout") {
                    logger::info("Clear Voice");
                    UnEquipFromActor(player, ActorSlot::Voice);
                    SendNotification("Cleared Voice");
#pragma endregion

#pragma region Map Controls
                }
                else if (currentCommand.Name == "open map") {
                    MenuInteraction(MenuType::Map, MenuAction::Open);
                    // SendNotification("Opening Map");
                }
                else if (currentCommand.Name == "close map") {
                    MenuInteraction(MenuType::Map, MenuAction::Close);
// SendNotification("Closing Map");
#pragma endregion

#pragma region Journal Controls
                }
                else if (currentCommand.Name == "open journal") {
                    MenuInteraction(MenuType::Journal, MenuAction::Open);
                    // SendNotification("Opening Journal");
                }
                else if (currentCommand.Name == "close journal") {
                    MenuInteraction(MenuType::Journal, MenuAction::Close);
                    // SendNotification("Closing Journal");
#pragma endregion

#pragma region Inventory Controls
                }
                else if (currentCommand.Name == "open inventory") {
                    MenuInteraction(MenuType::Inventory, MenuAction::Open);
                    // SendNotification("Opening Inventory");
                }
                else if (currentCommand.Name == "close inventory") {
                    MenuInteraction(MenuType::Inventory, MenuAction::Close);
                    // SendNotification("Closing Inventory");
#pragma endregion

#pragma region Spellbook Controls
                }
                else if (currentCommand.Name == "open spellbook") {
                    MenuInteraction(MenuType::Magic, MenuAction::Open);
                    // SendNotification("Opening Spellbook");
                }
                else if (currentCommand.Name == "close spellbook") {
                    MenuInteraction(MenuType::Magic, MenuAction::Close);
                    // SendNotification("Closing Spellbook");
#pragma endregion

#pragma region Skills Controls
                }
                else if (currentCommand.Name == "open skills") {
                    MenuInteraction(MenuType::Skills, MenuAction::Open);
                    // SendNotification("Opening Skills");
                }
                else if (currentCommand.Name == "close skills") {
                    MenuInteraction(MenuType::Skills, MenuAction::Close);
                    // SendNotification("Closing Skills");
#pragma endregion

#pragma region Level Up Controls
                }
                else if (currentCommand.Name == "open levelup") {
                    MenuInteraction(MenuType::LevelUp, MenuAction::Open);
                    // SendNotification("Opening Level Up");
#pragma endregion

#pragma region Save /Load Game
                }
                else if (currentCommand.Name == "quick save") {

                    //This is unsused, as a keybind works much better and more reliable

                    //Get "Quick Save" to work properly. I don't want to need to use keybinds for it. The commented out functionality is inconsistent on Flatrim and crashes VR

                    //player->GetCurrentLocation();
                    //const char* saveGameName = ("Voxima Quick Save - " + (std::string)player->GetCurrentLocation()->GetName()).c_str();
                    //RE::BGSSaveLoadManager::GetSingleton()->Save(saveGameName);

                    //PressKey(63); Alternative method of presing "F5"
                    

                    
                    //SendNotification("Saving Game");
                }
                else if (currentCommand.Name == "quick load") {
                    RE::BGSSaveLoadManager::GetSingleton()->LoadMostRecentSaveGame();
                    SendNotification("Loading Game");
                }
#pragma endregion

#pragma region Wait Controls
                else if (currentCommand.Name == "open wait") {
                    MenuInteraction(MenuType::SleepWait, MenuAction::Open);
                    // SendNotification("Opening Wait");
                }
                else if (currentCommand.Name == "close wait") {
                    MenuInteraction(MenuType::SleepWait, MenuAction::Close);
                    // SendNotification("Closing Wait");
                }
#pragma endregion

#pragma region Favorites Controls
                else if (currentCommand.Name == "open favorites") {
                    MenuInteraction(MenuType::Favorites, MenuAction::Open);
                    // SendNotification("Opening Favorites");
                }
                else if (currentCommand.Name == "close favorites") {
                    MenuInteraction(MenuType::Favorites, MenuAction::Close);
                    // SendNotification("Closing Favorites");
                }
#pragma endregion

#pragma region Favorites Controls
                else if (currentCommand.Name == "open console") {
                    MenuInteraction(MenuType::Console, MenuAction::Open);
                    // SendNotification("Opening Console");
                }
                else if (currentCommand.Name == "close console") {
                    MenuInteraction(MenuType::Console, MenuAction::Close);
                    // SendNotification("Closing Console");
                }
#pragma endregion
            }

            // Keybind
            else if (currentCommand.Type == "keybind") {
                if (currentCommand.Morph != "horseriding") {
                    if (currentCommand.KeybindDuration >= 0) {
                        PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                        SendNotification("Keybind Press: " + std::to_string(currentCommand.ID));
                    }
                    else
                        switch (currentCommand.KeybindDuration) {
                            case -1:
                                SendKeyDown(currentCommand.ID);
                                SendNotification("Keybind Hold: " + std::to_string(currentCommand.ID));
                                break;
                            case -2:
                                SendKeyUp(currentCommand.ID);
                                SendNotification("Keybind Release: " + std::to_string(currentCommand.ID));
                                break;
                            default:
                                SendNotification("ERROR: Invalid key duration: " + std::to_string(currentCommand.ID));
                                logger::error("Invalid key duration: {}", currentCommand.ID);
                                break;
                        }
                }
                else {
                    // Command is likely meant for a horse

                    // RE::BSInputDeviceManager* gamepad = RE::BSInputDeviceManager::GetSingleton();
                    // gamepad->GetGamepadHandler()->Initialize();

                    switch (currentCommand.ID) {
                        // W
                        case 17:
                            if (currentCommand.KeybindDuration == -1) {
                                MoveHorse(MoveType::MoveForward);
                                SendNotification("Horse: Move Forward");
                            }
                            else {
                                MoveHorse(MoveType::StopMoving);
                                SendNotification("Horse: Stop Moving");
                            }
                            break;

                            // A
                        case 30:
                            if (currentCommand.KeybindDuration == 0) {
                                MoveHorse(MoveType::MoveLeft);
                                SendNotification("Horse: Move Left");
                            }
                            else {
                                MoveHorse(MoveType::TurnLeft);
                                SendNotification("Horse: Turn Left");
                            }
                            break;

                            // S
                        case 31:
                            MoveHorse(MoveType::MoveBackward);
                            SendNotification("Horse: Turn Around");
                            break;

                            // D
                        case 32:
                            if (currentCommand.KeybindDuration == 0) {
                                MoveHorse(MoveType::MoveRight);
                                SendNotification("Horse: Move Right");
                            }
                            else
                                MoveHorse(MoveType::TurnRight);
                            SendNotification("Horse: Turn Right");
                            {}
                            break;

                            // Space
                        case 57:
                            MoveHorse(MoveType::MoveJump);
                            SendNotification("Horse: Jump");
                            break;

                            // Shift
                        case 42:
                            if (currentCommand.KeybindDuration == 0) {
                                MoveHorse(MoveType::MoveSprint);
                                SendNotification("Horse: Sprint");
                            }
                            else {
                                MoveHorse(MoveType::StopSprint);
                                SendNotification("Horse: Stop Sprinting");
                            }
                            break;

                        default:  // Not a command for controlling the horse
                            if (currentCommand.KeybindDuration >= 0)
                                PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                            else
                                switch (currentCommand.KeybindDuration) {
                                    case -1:
                                        SendKeyDown(currentCommand.ID);
                                        SendNotification("Keybind Hold: " + std::to_string(currentCommand.ID));
                                        break;
                                    case -2:
                                        SendKeyUp(currentCommand.ID);
                                        SendNotification("Keybind Release: " + std::to_string(currentCommand.ID));
                                        break;
                                    default:
                                        PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                                        SendNotification("Keybind Press: " + std::to_string(currentCommand.ID));
                                        break;
                                }
                    }
                }
            }

            // Console
            else if (currentCommand.Type == "console") {
                try {
                    SendNotification("Console: " + currentCommand.Name);
                    std::string command = currentCommand.Name;
                    std::string line = "";
                    std::string count = "";
                    std::vector<std::string> list;

                    for (int i = 0; i < command.length(); i++) {
                        for (line = ""; i < command.length() && command[i] != '+' && command[i] != '*'; i++) {
                            line += command[i];
                        }

                        if (command[i] == '*') {
                            for (i++, count = ""; i < command.length() && command[i] != '+'; i++) {
                                count += command[i];
                            }

                            for (int j = std::stoi(count); j > 0; j--) {
                                list.push_back(line);
                            }
                        }
                        else
                            list.push_back(line);

                    }  // End for

                    std::jthread console(ExecuteConsoleCommand, list);
                    console.detach();
                }
                catch (exception ex) {
                    SendNotification("ERROR: Invalid Command: " + currentCommand.Name);
                    logger::error("Invalid Console Command: {}\n{}", currentCommand.Name, ex.what());
                }
            }

            // Locations
            else if (currentCommand.Type == "location") {
                if (currentCommand.Name == "playerlocation")
                    NavigateToPlayer();
                else
                    NavigateToLocation(currentCommand.Name);

            }  // End type check
        });    // End Task
    }
}

#pragma region Event triggers for CheckUpdate()

#pragma region Animation Events

constexpr uint32_t exHash(const char* data, size_t const size) noexcept
{
    uint32_t hash = 5381;

    for (const char* c = data; c < data + size; ++c) {
        hash = ((hash << 5) + hash) + (unsigned char)*c;
    }

    return hash;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept { return exHash(str, size); }

// Executes when animation events occur
void Anim::Events::AnimationEvent(const char* holder, const char* name)
{
#pragma region Events of Interest

    // weaponDraw = take out weapon, spells, etc.
    // weaponSheathe = put away weapon, spells, etc.
    // MountEnd = occurs near beginning of mount horse or dragon
    // StopHorseCamera = occurs near the beginning of unmount horse or dragon

#pragma endregion Relevant events to track for UpdateCheck triggering

    string eventName(name);  // Convert passed-in argument to string
    /// logger::debug("{}: {}", holder, eventName);  // Output all the captured events to the log
    switch (exHash(name, eventName.size())) {
        case "weaponDraw"_h:
            logger::debug("Weapon has been drawn!!");
            break;
        case "weaponSheathe"_h:
            logger::debug("Weapon has been sheathed!!");
            break;
        case "MountEnd"_h:
            logger::debug("Player is now mounted!!");
            break;
        case "StopHorseCamera"_h:
            logger::debug("Player is now dismounted!!");
            // Stop all residual movement from horse controls
            SendKeyUp(17);  // Direction (N)
            SendKeyUp(32);  // Direction (E)
            SendKeyUp(31);  // Direction (S)
            SendKeyUp(30);  // Direction (W)
            SendKeyUp(30);  // Direction (W)
            SendKeyUp(42);  // Sprint    (Shift)
            break;
        default:
            return;
    }
    if (readyForUpdate == false) {  // Check if CheckUpdate should NOT fully execute
        logger::debug("Not ready for animation update!");
        return;
    }
    thread([]() {
        readyForUpdate = false;
        Sleep(500);  // Amount of time to disallow CheckUpdate from running again based on animation event triggers
        readyForUpdate = true;
    }).detach();
    CheckUpdate();  // Call method to check for game data updates

    /* if (eventName.find("weapon") != string::npos)
    {
        if (eventName == "weaponDraw" || eventName == "weaponSheathe") { // Check if animation event was related to weaponDraw or weaponSheathe
            logger::debug("Weapon has been drawn or sheathed!!");
            CheckUpdate();  // Call method to check for game data updates
        }
    } else if (eventName == "MountEnd" || eventName == "StopHorseCamera") { // Check if animation event was related to MountEnd or StopHorseCamera
        logger::debug("Player mount status changed!!");
        CheckUpdate();  // Call method to check for game data updates
    } */
}

#pragma endregion

// Executes when new spells are learned
void SpellLearnedEvent::EventHandler::SpellLearned(const RE::SpellItem* const spell)
{
    logger::debug("New spell learned!!");
    /// logger::info("Spell Learned = {}", spell->fullName.c_str());
    /// logger::info("Spell Learned = {}", spell->GetName());
    /// CheckUpdate();  // Call method to check for game data updates
}

// Executes when player's morph changes
void MorphEvents::EventHandler::MorphChanged()
{
    logger::debug("Player morph changed!!");
    CheckUpdate();  // Call method to check for game data updates
}

// Executes when game loads
void LoadGameEvent::EventHandler::GameLoaded()
{
    logger::debug("Game loaded!!");
    CheckUpdate();  // Call method to check for game data updates
}

// Executes when a menu opens or closes
void MenuOpenCloseEvent::EventHandler::MenuOpenClose(const RE::MenuOpenCloseEvent* event)
{
    string menuName = event->menuName.c_str();  // Capture name of triggering menu
    /// logger::info("Trigger Menu = {}!!", menuName);
    MenuType type;
    static std::unordered_map<std::string, MenuType> const table = {
        {"Console", MenuType::Console},      {"FavoritesMenu", MenuType::Favorites}, {"InventoryMenu", MenuType::Inventory},
        {"Journal Menu", MenuType::Journal}, {"LevelUp Menu", MenuType::LevelUp},    {"MagicMenu", MenuType::Magic},
        {"MapMenu", MenuType::Map},          {"StatsMenu", MenuType::Skills},        {"Sleep/Wait Menu", MenuType::SleepWait},
        {"TweenMenu", MenuType::Tween},
    };
    auto it = table.find(menuName);
    if (it != table.end())  // Check if match was found within enums
        type = it->second;
    else  // No enum match found
        return;
    switch (type) {  // Check if triggering menu is of interest
        case MenuType::Console:
            menuName = RE::Console::MENU_NAME;
            break;

        case MenuType::Favorites:
            menuName = RE::FavoritesMenu::MENU_NAME;
            break;

        case MenuType::Inventory:
            menuName = RE::InventoryMenu::MENU_NAME;
            break;

        case MenuType::Journal:
            menuName = RE::JournalMenu::MENU_NAME;
            break;

        case MenuType::LevelUp:
            menuName = RE::LevelUpMenu::MENU_NAME;
            break;

        case MenuType::Magic:
            menuName = RE::MagicMenu::MENU_NAME;
            break;

        case MenuType::Map:
            menuName = RE::MapMenu::MENU_NAME;

            if (event->opening)
                SendMessage("enable location commands");
            else
                SendMessage("disable location commands");
            break;

        case MenuType::Skills:
            menuName = RE::StatsMenu::MENU_NAME;
            break;

        case MenuType::SleepWait:
            menuName = RE::SleepWaitMenu::MENU_NAME;
            break;

        case MenuType::Tween:
            menuName = RE::TweenMenu::MENU_NAME;
            break;

        default:  // Triggering menu is not of interest
            logger::error("Error processing menu event - unexpected enum encountered");
            return;
    }
    // if (event->opening == true) {  // Check if captured event involves menu OPENING
    //     logger::info("OPEN Menu = {}!!", menuName);
    // } else {  // Captured event involves menu CLOSING
    //     logger::debug("CLOSED Menu = {}!!", menuName);
    // }
    CheckUpdate();  // Call method to check for game data updates

    // string menuName = event->menuName.c_str();  // Capture name of closed menu
    // if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if trigger menu is of interest
    //     logger::info("Trigger Menu = {}!!", menuName);
    //     if (event->opening == true) // Check if captured event involves menu OPENING
    //         logger::debug("OPENED Menu = {}!!", menuName);
    //     else
    //         logger::debug("CLOSED Menu = {}!!", menuName);
    //     CheckUpdate();  // Call method to check for game data updates
    // }

    ///// logger::debug("Menu opened or closed!!");
    // if (event->opening == false) {  // Check if capture event involves menu CLOSING
    //     string menuName = event->menuName.c_str(); // Capture name of closed menu
    //     /// logger::info("CLOSE Menu = {}!!", menuName);
    //     if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if menu of interest was closed
    //         logger::debug("CLOSED Menu = {}!!", menuName);
    //         CheckUpdate(); // Call method to check for game data updates
    //     }
    // } else {
    //     string menuName = event->menuName.c_str();  // Capture name of opened menu
    //     /// logger::info("OPEN Menu = {}!!", menuName);
    //     if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if menu of interest was closed
    //         logger::debug("CLOSED Menu = {}!!", menuName);
    //         CheckUpdate();  // Call method to check for game data updates
    //     }
    // }
}

// Executes when new locations are discovered
void LocationDiscoveredEvent::EventHandler::LocationDiscovered(string locationName)
{
    logger::debug("{} discovered", locationName);
    CheckUpdate();

    ///*** do something with locationName or call GetKnownLocations()

    // do other stuff
}

#pragma endregion All tracked game events that trigger an UpdateCheck

#pragma region Flatrim Device Input Processing

bool pushToSpeakListening = false;

// Executes when "Flatrim" (non-VR Skyrim) input device events are received
void DeviceInputEvent::DeviceInputHandler::FlatrimInputDeviceEvent(RE::ButtonEvent* button, uint32_t keyCode)
{
    /*
        EXAMPLE "KEY" TRIGGERS
        Keyboard backslash (\) = 43
        Mouse middle button = 258
        Gamepad right shoulder button = 275
    */

    /*
    VOX_PushToSpeakType:
        [0] = Disabled
        [1] = Hold
        [2] = Toggle
        [3] = Vocal
      */

    if (VOX_PushToSpeakType->value == 0 ||
        keyCode != VOX_PushToSpeakKeyCode->value)  // Check if PushToSpeak is disabled OR triggering keyCode does NOT match target input value from VOX MCM
        return;                                    // Exit this method

    if (VOX_PushToSpeakType->value == 1) {  // Button must be Held
        thread([button]() {                 // Create new thread for execution (passing in button)
            SendNotification("Flatrim Input Triggered - Start listening!");
            SendMessage(WebSocketMessage::EnableRecognition);
            while (button->IsPressed()) Sleep(250);  // Pause while input trigger is still being pressed
            SendNotification("Flatrim Input released - Stop Listening!");
            SendMessage(WebSocketMessage::DisableRecognition);
        })
            .detach();
    }
    else if (VOX_PushToSpeakType->value == 2) {  // Button toggles
        thread([button]() {                      // Create new thread for execution (passing in button)
            if (!pushToSpeakListening) {         // Check if recognition app is NOT listening
                pushToSpeakListening = true;     // Set isRecognitionEnabled flag
                SendNotification("Flatrim Input Triggered - Start listening!");
                SendMessage(WebSocketMessage::EnableRecognition);
            }
            else {
                pushToSpeakListening = false;  // Reset isRecognitionEnabled flag
                SendNotification("Flatrim Input released - Stop Listening!");
                SendMessage(WebSocketMessage::DisableRecognition);
            }
            while (button->IsPressed()) Sleep(250);  // Pause while input trigger is still being pressed
        })
            .detach();
    }
}

#pragma endregion "Flatrim" device input triggers for speech recognition listening


// Initializes Plugin, Speech Recognition, Websocket, and data tracker
SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    try {
        SKSE::Init(skse);  // Initialize SKSE plugin
        /// MessageBoxA(NULL, "Press OK when you're ready!", "Skyrim", MB_OK | MB_ICONQUESTION);  // MessageBox to halt execution so a debugger can be attached
        SetupLog();  // Set up the debug logger

        SKSE::GetMessagingInterface()->RegisterListener(OnMessage);  // Listen for game messages (usually "Is Game Loaded" comes first) and execute OnMessage method in response
        logger::info("Voxima finished loading");                     // Temporary debug line so I can see the program started
        return true;
    }
    catch (exception ex) {
        logger::info("ERROR initializing Voxima SKSE plugin: {}", ex.what());
        return false;
    }
}  // End SKSEPluginLoad

// Notes
/*
(1) I may be able to change this an event that is triggered, so if VOX_Enabled is 0, the loop will break. If VOX_Enabled is set to 1, the function is
called again, starting the loop

*/

#pragma region Archive

//    May need in the future
/* struct OurEventSink : public RE::BSTEventSink<RE::TESBookReadEvent> {
    RE::BSEventNotifyControl ProcessEvent(const RE::BSTEventSink<RE::TESBookReadEvent>event, RE::BSTEventSink<RE::TESBookReadEvent>* source) {
        //auto* player = RE::TESForm::LookupByID<RE::Actor>(14);
        logger::info("{}", event);

        //auto* spellList = player->GetActorBase()->GetSpellList();
        //
        //
        //if (!spellList) {
        //  return RE::BSEventNotifyControl::kContinue;
        //}
        //
        //for (int i = 0; i < spellList->numSpells; i++) {
        //  logger::info("{}", spellList->spells[i]->GetFullName());


        return RE::BSEventNotifyControl::kContinue;
    }
}; */

/* class EventProcessor : public RE::BSTEventSink<RE::TESActivateEvent>,
                       public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
                       public RE::BSTEventSink<RE::TESBookReadEvent>
{
    // Pretty typical singleton setup
    // *Private* constructor/destructor
    // And we *delete* the copy constructors and move constructors.
    EventProcessor() = default;
    ~EventProcessor() = default;
    EventProcessor(const EventProcessor&) = delete;
    EventProcessor(EventProcessor&&) = delete;
    EventProcessor& operator=(const EventProcessor&) = delete;
    EventProcessor& operator=(EventProcessor&&) = delete;

    public:
        // Returns a reference to the one and only instance of EventProcessor :)
        //
        // Note: this is returned as a & reference. When you need this as a pointer, you'll want to use & (see below)
        static EventProcessor& GetSingleton() {
            static EventProcessor singleton;
            return singleton;
        }

    // Log information about Activate events that happen in the game
    RE::BSEventNotifyControl ProcessEvent(const RE::TESActivateEvent* event, RE::BSTEventSource<RE::TESActivateEvent>*) override {
        logger::info("{} activated {}", event->actionRef.get()->GetBaseObject()->GetName(), event->objectActivated.get()->GetBaseObject()->GetName());
        return RE::BSEventNotifyControl::kContinue;
    }

    // Log information about Menu open/close events that happen in the game
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        logger::info("Menu {} Open? {}", event->menuName, event->opening);
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESBookReadEvent* event, RE::BSTEventSource<RE::TESBookReadEvent>*) {
        ////auto* player = RE::TESForm::LookupByID<RE::Actor>(14);
        ///logger::info("{}", event);

        auto* spellList = player->GetActorBase()->GetSpellList();
        if (!spellList) {
            return RE::BSEventNotifyControl::kContinue;
        }
        for (int i = 0; i < spellList->numSpells; i++) {
            logger::info("{}", spellList->spells[i]->GetFullName());
        }
        return RE::BSEventNotifyControl::kContinue;
    }
}; */

/* // Executes when gamepad events are received
void GamepadInputEvent::GamepadInputHandler::GamepadEvent(RE::BSWin32GamepadDevice* gamepad) {
    auto rShoulder = RE::BSWin32GamepadDevice::Keys::kRightShoulder;
    if (allowTriggerProcessing && gamepad->IsPressed(rShoulder)) {  // Check if gamepad trigger processing is currently allowed and the trigger is being pressed
        allowTriggerProcessing = false;                             // Flag that gamepad trigger processing is NOT allowed
        if (pushToTalk) {
            thread([gamepad, rShoulder]() {                         // Create new thread for execution
                SendNotification("Start listening!");

                /// *** do stuff to activate listening of C# app

                while (gamepad->IsPressed(rShoulder)) Sleep(250);   // Pause while gamepad trigger is still being pressed
                SendNotification("Stop listening!");
                allowTriggerProcessing = true;                      // Flag that gamepad trigger processing is allowed

                /// *** do stuff to deactivate listening of C# app

            }).detach();
        } else {
            thread([gamepad, rShoulder]() {                         // Create new thread for execution
                if (isListening == false) {                         // Check if recognition app is NOT listening
                    isListening = true;                             // Set isListening flag
                    SendNotification("Start listening!");

                    /// *** do stuff to activate listening of C# app

                } else {
                    isListening = false;                            // Reset isListening flag
                    SendNotification("Stop listening!");

                    /// *** do stuff to deactivate listening of C# app
                }
                while (gamepad->IsPressed(rShoulder)) Sleep(250);  // Pause while gamepad trigger is still being pressed
                allowTriggerProcessing = true;                     // Flag that gamepad trigger processing is allowed
            }).detach();
        }
    }
} */

#pragma endregion

#pragma region References

// Hash parttern for switch statements modified from TrueDirectionalMovement by Ersh ==> https://github.com/ersh1/TrueDirectionalMovement
// string to enum ==> https://stackoverflow.com/questions/7163069/c-string-to-enum/7163130#7163130

// Leverages SkyrimVRTools plugin (required mod) https://www.nexusmods.com/skyrimspecialedition/mods/27782
// Leverages code from VRCustomQuickslots https://github.com/lfrazer/VRCustomQuickslots

#pragma endregion