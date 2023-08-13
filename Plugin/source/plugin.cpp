
// Include statements
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
#include "../functions/SkyrimMessageBox.hpp"       // Miscellaneous custom functions
#include "../events/animation-events.hpp"          // Animation event hooking and processing
#include "../events/spell-learned-event.hpp"       // Spell learn event hooking and processing
#include "../events/morph-changed-event.hpp"       // Player morph event hooking and processing
#include "../events/load-game-event.hpp"           // Game load event hooking and processing
#include "../events/menu-open-close-event.hpp"     // Menu close event hooking and processing
#include "../events/location-discovery-event.hpp"  // Location discovery event hooking and processing
#include "../events/device-input-event.hpp"        // Flatrim device input event hooking and processing
#include "../functions/logger.hpp"                 // SKSE log functions

#pragma region Structs
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
#pragma endregion

#pragma region Function Definitions
void InitialUpdate();
void CheckUpdate(bool loop = false);
void Update(std::string update = "");
void ExecuteCommand(Command command);
void OnVRButtonEvent(PapyrusVR::VREventType type, PapyrusVR::EVRButtonId buttonId, PapyrusVR::VRDevice deviceId);
#pragma endregion

#pragma region Global Variables
std::string id = "";
int currentPushToSpeakType;
float currentSensitivity;
float currentAutoCastShouts;
float currentAutoCastPowers;
uint32_t numMagic[] = {0, 0};
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
    try
    {
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

        switch (message->type)
        {
            // All plugins have been loaded
            #pragma region All Plugins Loaded
            case SKSE::MessagingInterface::kPostLoad:
                ConfigureWebsocketPort();  // Write target websocket port to file, which will be read by speech recognition application
                LaunchSpeechRecoApp();     // Launch the companion speech recognition application

                if (REL::Module::IsVR())
                {
                    logger::debug("SKSE PostLoad message received, registering for PapyrusVR messages from SkyrimVRTools");
                    SKSE::GetMessagingInterface()->RegisterListener("SkyrimVRTools", OnPapyrusVRMessage);
                }
                break;
            #pragma endregion

            // Data handler has loaded all its forms (Main menu has loaded)
            #pragma region Main Menu has Loaded
            case SKSE::MessagingInterface::kDataLoaded:
                InitializeWebSocketClient();           // Initialize the websocket client owned by the plugin
                InitializeLoadGameHooking();           // Setup game load event monitoring
                InitializeAnimationHooking();          // Setup animation event monitoring
                InitializeSpellLearnHooking();         // Setup spell learn event monitoring
                InitializeMorphChangeHooking();        // Setup player morph change event monitoring
                InitializeMenuOpenCloseHooking();      // Setup menu open/close event monitoring
                InitializeLocationDiscoveryHooking();  // Setup location discovery event monitoring
                if (REL::Module::IsVR())
                {
                    if (g_papyrusvr)
                    {
                        OpenVRHookManagerAPI* hookMgrAPI = RequestOpenVRHookManagerObject();
                        hookMgrAPI = NULL;
                        if (hookMgrAPI)
                        {
                            logger::debug("Using RAW OpenVR Hook API.");
                            g_VRSystem = hookMgrAPI->GetVRSystem();  // setup VR system before callbacks
                            hookMgrAPI->RegisterControllerStateCB(OnControllerStateChanged);
                            // hookMgrAPI->RegisterGetPosesCB(OnGetPosesUpdate);
                        }
                        else
                        {
                            logger::debug("Using legacy PapyrusVR API.");

                            // Registers for PoseUpdates
                            g_papyrusvr->GetVRManager()->RegisterVRButtonListener(OnVRButtonEvent);
                            // g_papyrusvr->GetVRManager()->RegisterVRUpdateListener(OnVRUpdateEvent);
                        }
                    }
                    else
                    {
                        logger::debug("PapyrusVR was not initialized!");
                    }
                }
                else
                    InitializeFlatrimDeviceInputHooking();                        // Setup "Flatrim" (non-VR Skyrim) device input event monitoring
                while (connected == false)                                        // Loop while websocket connection has not been made
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Brief pause to allow for websocket connection to be made
                InitialUpdate();                                                  // Run initial data update
                break;
            #pragma endregion

            // Save Game as been created (manual, autosave, or new game)
            #pragma region Save Game Created
            case SKSE::MessagingInterface::kSaveGame:
                if (saveTriggered == false)
                {
                    saveTriggered = true;
                    logger::debug("Save game created!!");
                    CheckUpdate();
                }
                break;
            #pragma endregion

        }  // End switch
    }
    catch (const std::exception& ex)
    {
        logger::error("ERROR during OnMessage: {}", ex.what());
    }
}

// Initial game data gathering
void InitialUpdate()
{
    try
    {
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
        VOX_Sensitivity = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_Sensitivity");
        VOX_QuickLoadType = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_QuickLoadType");

        //currentSensitivity = VOX_Sensitivity->value;
        //currentAutoCastShouts = VOX_AutoCastShouts->value;
        //currentAutoCastPowers = VOX_AutoCastPowers->value;
    }
    catch (const std::exception& ex)
    {
        logger::error("ERROR during InitialUpdate: {}", ex.what());
    }
}

// Checks for changes in the tracked data
void CheckUpdate(bool loop)
{
    try
    {
        thread([loop]() {
            logger::debug("CheckUpdate method running");
            bool fullUpdate;
            int placeInQueue = updateQueue;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (placeInQueue < updateQueue) return;

            updateQueue++;

            if (loop) logger::debug("CheckUpdate loop start");

            do
            {
                if (!player->Is3DLoaded())
                {
                    if (loop) std::this_thread::sleep_for(std::chrono::seconds(1));  // 1 second delay to avoid looping unnecessarily
                    continue;
                }
                else if (VOX_Enabled->value == 0)
                {
                    if (loop)
                        std::this_thread::sleep_for(std::chrono::seconds(5));  // 5 second delay to not loop unnecessarily. See Bottom of page Note (1)
                    else
                    {
                        SendMessage(WebSocketMessage::DisableRecognition);
                        isRecognitionEnabled = false;
                    }

                    continue;
                }
                else if (VOX_Enabled->value == 1 && !isRecognitionEnabled)
                {
                    SendMessage(WebSocketMessage::EnableRecognition);
                    isRecognitionEnabled = true;
                }

                fullUpdate = false;

                #pragma region Check for Update

                // Checks for change in number of magic items (includes effects)
                if (numMagic[0] != player->GetActorRuntimeData().addedSpells.size())
                {
                    numMagic[0] = player->GetActorRuntimeData().addedSpells.size();

                    // Checks for a change in the number Spells or Powers
                    uint32_t currentSpells = 0;
                    for (RE::SpellItem* item : player->GetActorRuntimeData().addedSpells)
                    {
                        switch (item->GetSpellType())
                        {
                            case RE::MagicSystem::SpellType::kSpell:
                            case RE::MagicSystem::SpellType::kPower:
                            case RE::MagicSystem::SpellType::kLesserPower:
                            case RE::MagicSystem::SpellType::kVoicePower:
                                currentSpells++;
                                break;
                        }  // End Switch
                    }

                    // If there is a change, update the recorded total and mark for Update()
                    if (currentSpells != numMagic[1])
                    {
                        numMagic[1] = currentSpells;
                        fullUpdate = true;
                    }
                }

                std::string newCSharpSettings = "";
                //-----MCM-----//
                #pragma region MCM Settings
                if (currentPushToSpeakType != (int)VOX_PushToSpeakType->value)
                {
                    if (currentPushToSpeakType == 3 || VOX_PushToSpeakType->value == 3)
                        logger::info("Sending \"VOX_VocalPushToSpeak = {}\" to C#", VOX_PushToSpeakType->value);

                    currentPushToSpeakType = VOX_PushToSpeakType->value;

                    switch (currentPushToSpeakType)
                    {
                        case 1:
                        case 2:
                            SendMessage(WebSocketMessage::DisableRecognition);
                            newCSharpSettings += "VOX_VocalPushToSpeak\tfalse\n";
                            break;
                        case 0:
                            SendMessage(WebSocketMessage::EnableRecognition);
                            newCSharpSettings += "VOX_VocalPushToSpeak\tfalse\n";
                            break;
                        case 3:
                            SendMessage(WebSocketMessage::EnableRecognition);
                            newCSharpSettings += "VOX_VocalPushToSpeak\ttrue\n";
                            break;
                    }
                }

                if (currentSensitivity != VOX_Sensitivity->value)
                {
                    currentSensitivity = VOX_Sensitivity->value;
                    logger::info("Sending \"VOX_Sensitivity = {}\" to C#", VOX_Sensitivity->value);

                    newCSharpSettings += "VOX_Sensitivity\t" + std::to_string(VOX_Sensitivity->value) + "\n";
                }

                if (currentAutoCastShouts != VOX_AutoCastShouts->value)
                {
                    currentAutoCastShouts = VOX_AutoCastShouts->value;
                    logger::info("Sending \"VOX_AutoCastShouts = {}\" to C#", VOX_AutoCastShouts->value);

                    newCSharpSettings += "VOX_AutoCastShouts\t" + (currentAutoCastShouts != 0 ? std::string("true") : std::string("false")) + "\n";
                }

                if (currentAutoCastPowers != VOX_AutoCastPowers->value)
                {
                    currentAutoCastPowers = VOX_AutoCastPowers->value;
                    logger::info("Sending \"VOX_AutoCastPowers = {}\" to C#", VOX_AutoCastPowers->value);

                    newCSharpSettings += "VOX_AutoCastPowers\t" + (currentAutoCastPowers != 0 ? std::string("true") : std::string("false")) + "\n";
                }

                if (VOX_CheckForUpdate->value == 1)
                {
                    SendMessage(WebSocketMessage::CheckForMicChange);
                    VOX_CheckForUpdate->value = 0;
                    logger::info("Forcing an Update Check");
                    currentShouts = "";
                    fullUpdate = true;
                }
                #pragma endregion

                if (newCSharpSettings != "") SendMessage(WebSocketMessage::UpdateConfiguration + newCSharpSettings);

                knownShouts = GetActorMagic(player, MagicType::Shout)[0];  // Obtain currently known shouts

                if (knownShouts != currentShouts)
                {  // Check if there is an update to the shouts
                    currentShouts = knownShouts;
                    logger::info("Updating Shouts");
                    SendMessage(WebSocketMessage::UpdateShouts + knownShouts);
                    fullUpdate = true;
                }  // End Shout Check

                knownLocations = GetKnownLocations();
                if (currentLocations != knownLocations)
                {
                    currentLocations = knownLocations;
                    std::string locationsMessage = "";
                    std::string currentMarker;

                    for (std::string playerMapMarker : currentLocations)
                    {
                        locationsMessage += playerMapMarker + "\n";
                    }
                    logger::info("{}", locationsMessage);

                    SendMessage(WebSocketMessage::UpdateLocations + locationsMessage + "playerlocation");
                }

                if (fullUpdate) Update();

                #pragma endregion

                if (loop) std::this_thread::sleep_for(std::chrono::seconds((int)VOX_UpdateInterval->value));  // Pause based on VOX MCM settings
            } while (loop);                                                                                   // End While
        }).detach();
    }
    catch (const std::exception& ex)
    {
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
    try
    {
        thread([update]() {
            logger::info("Updating Information");
            bool finished = false;
            std::string type = update;
            std::string updateFile = "";
            std::string updateSpells = "";
            std::string updatePowers = "";
            std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });  // sets type.ToLower()

            if (!finished)
            {
                auto magic = GetActorMagic(player, MagicType::Spell, MagicType::Power);

                SendMessage(WebSocketMessage::UpdateSpells + magic[0]);
                SendMessage(WebSocketMessage::UpdatePowers + magic[1]);
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(200));  // Brief pause to ensure the following message is sent and processed after the previous messages
            SendMessage(WebSocketMessage::InitializeUpdate);
        }).detach();
    }
    catch (const std::exception& ex)
    {
        logger::error("ERROR during Update:\n{}\n", ex.what());
    }
}

// Process messages received from websocket server
void ProcessReceivedMessage(const string& command)
{
    try
    {
        // Shorten display of message to fit on a single line, for display purposes
        std::string messagePrint = command;
        std::string oldSubstr = "\n";
        std::string newSubstr = "\\n";

        // Replace all occurrences of oldSubstr with newSubstr
        size_t pos = 0;
        while ((pos = messagePrint.find(oldSubstr, pos)) != std::string::npos)
        {
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
        for (line = "", j = 0; i < command.length(); j++)
        {
            for (; i < command.length() && (command[i] == '\r' || command[i] == '\n'); i++)
                ;

            for (line = ""; i < command.length() && command[i] != '\r' && command[i] != '\n'; i++)
            {
                line += command[i];
            }

            /// logger::info("{}", line);

            switch (j)
            {
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

        switch (PlayerMorph())
        {
            case 0:
                if (currentCommand.Morph != "none")
                {
                    logger::info("Command Ignored. Mismatched Morph. (Current: none) (Command: {})", currentCommand.Morph);
                    return;
                }
                break;

            case 1:
                if (currentCommand.Morph != "werewolf")
                {
                    logger::info("Command Ignored. Mismatched Morph. (Current: werewolf) (Command: {})", currentCommand.Morph);
                    return;
                }
                break;

            case 2:
                if (currentCommand.Morph != "vampirelord")
                {
                    logger::info("Command Ignored. Mismatched Morph. (Current: vampirelord) (Command: {})", currentCommand.Morph);
                    return;
                }
                break;
        }

        logger::info("Executing Command");

        ExecuteCommand(currentCommand);
    }
    catch (exception ex)
    {
        logger::error("ERROR while preprocessing\n\"{}\"\nmessage from server: \"{}\"", command, ex.what());
    }
}

// Executes a given command
void ExecuteCommand(Command command)
{
    const auto task = SKSE::GetTaskInterface();

    if (task != nullptr)
    {
        task->AddTask([=]() {
            Command currentCommand = command;  // Passed-in "command" argument cannot be modified, so transfer contents to modifiable currentCommand

// Spell
#pragma region Spell
            if (currentCommand.Type == "spell")
            {
                id = std::format("{:X}", currentCommand.ID, nullptr, 16);

                for (; id.length() < 6; id = '0' + id)
                    ;

                currentCommand.ID =
                    std::stoi(std::format("{:X}", RE::TESDataHandler::GetSingleton()->GetModIndex(currentCommand.fileName).value()) + id, nullptr, 16);

                RE::SpellItem* item = RE::TESForm::LookupByID<RE::SpellItem>(currentCommand.ID);

                if (item->GetName() == nullptr || item->GetName() == "")
                {
                    logger::info("Spell is NULL");
                    return;
                }

                RE::SpellItem* spell = item->As<RE::SpellItem>();
                bool allowedChargeTime = spell->data.chargeTime <= 1 || VOX_LongAutoCast->value == 1;
                bool allowedCastingType = item->As<RE::MagicItem>()->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget;

                bool validCastingItem = allowedCastingType && allowedChargeTime;

                logger::info("Spell Command Identified: Name: \"{}\"     Hand: \"{}\"", item->GetName(), currentCommand.Hand);

                switch (currentCommand.Hand)
                {
                    // Left
                    case 0:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Left))
                        {
                            EquipToActor(player, item, ActorSlot::Left);
                        }
                        break;

                    // Right
                    case 1:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Right))
                        {
                            EquipToActor(player, item, ActorSlot::Right);
                        }
                        break;

                    // Both
                    case 2:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Both))
                        {
                            EquipToActor(player, item, ActorSlot::Both);
                        }
                        break;

                    // Left with Perk
                    case 3:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Left, 1))
                        {
                            EquipToActor(player, item, ActorSlot::Left);
                        }
                        break;

                    // Right with Perk
                    case 4:
                        if (!validCastingItem || !currentCommand.AutoCast || !CastMagic(player, item, ActorSlot::Right, 1))
                        {
                            EquipToActor(player, item, ActorSlot::Right);
                        }
                        break;
                }  // End Switch

                // Clear Hands
            }
#pragma endregion

// Power
#pragma region Power
            else if (currentCommand.Type == "power")
            {
                id = std::format("{:X}", currentCommand.ID, nullptr, 16);

                for (; id.length() < 6; id = '0' + id)
                    ;

                currentCommand.ID =
                    std::stoi(std::format("{:X}", RE::TESDataHandler::GetSingleton()->GetModIndex(currentCommand.fileName).value()) + id, nullptr, 16);

                RE::SpellItem* item = RE::TESForm::LookupByID<RE::SpellItem>(currentCommand.ID);

                if (item->GetName() == NULL || item->GetName() == "")
                {
                    logger::info("Power is NULL");
                    return;
                }

                logger::info("Power Command Identified: Name: \"{}\"", item->GetName());

                if (currentCommand.AutoCast)
                    CastMagic(player, item, ActorSlot::Voice);
                else
                    EquipToActor(player, item, ActorSlot::Voice);
            }
#pragma endregion

// Shout
#pragma region Shout
            else if (currentCommand.Type == "shout")
            {
                id = std::format("{:X}", currentCommand.ID, nullptr, 16);

                for (; id.length() < 6; id = '0' + id)
                    ;

                currentCommand.ID =
                    std::stoi(std::format("{:X}", RE::TESDataHandler::GetSingleton()->GetModIndex(currentCommand.fileName).value()) + id, nullptr, 16);

                RE::TESShout* item = RE::TESForm::LookupByID<RE::TESShout>(currentCommand.ID);

                if (item->GetName() == NULL || item->GetName() == "")
                {
                    logger::info("Shout is NULL");
                    return;
                }

                logger::info("Shout Command Identified: Name: \"{}\"     Level: \"{}\"", item->GetName(),
                             currentCommand.Hand + 1);  // Hand meanings: [0] == Level 1   [1] == Level 2   [2] = Level 3

                if (currentCommand.AutoCast)
                    CastMagic(player, item, ActorSlot::Voice, currentCommand.Hand);
                else
                    EquipToActor(player, item, ActorSlot::Voice);
            }
#pragma endregion

            // Setting
            else if (currentCommand.Type == "setting")
            {
#pragma region UI and Menu Controls

#pragma region Map Controls
                if (currentCommand.Name == "open map")
                {
                    MenuInteraction(MenuType::Map, MenuAction::Open);
                    // SendNotification("Opening Map");
                }
                else if (currentCommand.Name == "close map")
                {
                    MenuInteraction(MenuType::Map, MenuAction::Close);
                    // SendNotification("Closing Map");
                }
#pragma endregion

#pragma region Journal Controls
                else if (currentCommand.Name == "open journal")
                {
                    MenuInteraction(MenuType::Journal, MenuAction::Open);
                    // SendNotification("Opening Journal");
                }
                else if (currentCommand.Name == "close journal")
                {
                    MenuInteraction(MenuType::Journal, MenuAction::Close);
                    // SendNotification("Closing Journal");
                }
#pragma endregion

#pragma region Inventory Controls
                else if (currentCommand.Name == "open inventory")
                {
                    MenuInteraction(MenuType::Inventory, MenuAction::Open);
                    // SendNotification("Opening Inventory");
                }
                else if (currentCommand.Name == "close inventory")
                {
                    MenuInteraction(MenuType::Inventory, MenuAction::Close);
                    // SendNotification("Closing Inventory");
                }
#pragma endregion

#pragma region Spellbook Controls
                else if (currentCommand.Name == "open spellbook")
                {
                    if (PlayerMorph() != 2)
                        MenuInteraction(MenuType::Magic, MenuAction::Open);
                    else
                        MenuInteraction(MenuType::Favorites, MenuAction::Open);

                    // SendNotification("Opening Spellbook");
                }
                else if (currentCommand.Name == "close spellbook")
                {
                    if (PlayerMorph() != 2)
                        MenuInteraction(MenuType::Magic, MenuAction::Close);
                    else
                        MenuInteraction(MenuType::Favorites, MenuAction::Close);

                    // SendNotification("Closing Spellbook");
                }
#pragma endregion

#pragma region Skills Controls
                else if (currentCommand.Name == "open skills")
                {
                    MenuInteraction(MenuType::Skills, MenuAction::Open);

                    // SendNotification("Opening Skills");
                }
                else if (currentCommand.Name == "close skills")
                {
                    MenuInteraction(MenuType::Skills, MenuAction::Close);
                    ///if (PlayerMorph() != 2) `MenuInteraction(MenuType::Skills, MenuAction::Close);

                    // SendNotification("Closing Skills");
                }
#pragma endregion

#pragma region Level Up Controls
                else if (currentCommand.Name == "open levelup")
                {
                    MenuInteraction(MenuType::LevelUp, MenuAction::Open);
                    // SendNotification("Opening Level Up");
                }
#pragma endregion

#pragma region Favorites Controls
                else if (currentCommand.Name == "open favorites")
                {
                    MenuInteraction(MenuType::Favorites, MenuAction::Open);
                    // SendNotification("Opening Favorites");
                }
                else if (currentCommand.Name == "close favorites")
                {
                    MenuInteraction(MenuType::Favorites, MenuAction::Close);
                    // SendNotification("Closing Favorites");
                }
#pragma endregion

#pragma region Wait Controls
                else if (currentCommand.Name == "open wait")
                {
                    MenuInteraction(MenuType::SleepWait, MenuAction::Open);
                    // SendNotification("Opening Wait");
                }
                else if (currentCommand.Name == "close wait")
                {
                    MenuInteraction(MenuType::SleepWait, MenuAction::Close);
                    // SendNotification("Closing Wait");
                }
#pragma endregion

#pragma region Console Controls
                else if (currentCommand.Name == "open console")
                {
                    MenuInteraction(MenuType::Console, MenuAction::Open);
                    // SendNotification("Opening Console");
                }
                else if (currentCommand.Name == "close console")
                {
                    MenuInteraction(MenuType::Console, MenuAction::Close);
                    // SendNotification("Closing Console");
                }
#pragma endregion

#pragma endregion

#pragma region Werewolf Shout
                else if (currentCommand.Name == "werewolf shout")
                {
                    if (PlayerMorph() == 1)
                    {
                        // Spoof button input to perform a "werewolf shout"
                        if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton())
                        {
                            auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 1.0f, 0.0f);
                            bsInputEventQueue->PushOntoInputQueue(kEvent);
                        }
                        //CastVoice(player, nullptr, 0);
                    }
                }

#pragma endregion

#pragma region Horse Controls
#pragma region Move Forward /Stop
                else if (currentCommand.Name == "horse - forward")
                {
                    MoveHorse(MoveType::MoveForward);
                }
                else if (currentCommand.Name == "horse - stop")
                {
                    MoveHorse(MoveType::StopMoving);
                }
#pragma endregion

#pragma region Sprinting
                else if (currentCommand.Name == "horse - sprint")
                {
                    MoveHorse(MoveType::MoveSprint);
                }
                else if (currentCommand.Name == "horse - stop sprint")
                {
                    MoveHorse(MoveType::StopSprint);
                }
#pragma endregion

#pragma region Run
                else if (currentCommand.Name == "horse - run")
                {
                    MoveHorse(MoveType::MoveRun);
                }
                else if (currentCommand.Name == "horse - stop run")
                {
                    MoveHorse(MoveType::MoveWalk);
                }
#pragma endregion

#pragma region Walk
                else if (currentCommand.Name == "horse - walk")
                {
                    MoveHorse(MoveType::MoveWalk);
                }
                else if (currentCommand.Name == "horse - stop walk")
                {
                    RE::ActorPtr horse;

                    if (player == nullptr) return;

                    (void)player->GetMount(horse);

                    if (horse == nullptr)
                    {
                        SendNotification("No Mount Detected");
                        return;
                    }

                    if (IsActorWalking(horse)) MoveHorse(MoveType::StopMoving);
                }
#pragma endregion

#pragma region Turning Left /Right
                else if (currentCommand.Name == "horse - 90 left")
                {
                    MoveHorse(MoveType::MoveLeft);
                }
                else if (currentCommand.Name == "horse - 45 left")
                {
                    MoveHorse(MoveType::TurnLeft);
                }
                else if (currentCommand.Name == "horse - 90 right")
                {
                    MoveHorse(MoveType::MoveRight);
                }
                else if (currentCommand.Name == "horse - 45 right")
                {
                    MoveHorse(MoveType::TurnRight);
                }
                else if (currentCommand.Name == "horse - turn around")
                {
                    MoveHorse(MoveType::TurnAround);
                }
#pragma endregion

#pragma region Jumping
                else if (currentCommand.Name == "horse - jump")
                {
                    RE::ActorPtr horse;

                    if (player == nullptr) return;

                    (void)player->GetMount(horse);

                    if (horse == nullptr)
                    {
                        SendNotification("No Mount Detected");
                        return;
                    }

                    if (horse->IsMoving()) MoveHorse(MoveType::MoveJump);
                }
#pragma endregion

#pragma region Rear Up
                else if (currentCommand.Name == "horse - rear up")
                {
                    RE::ActorPtr horse;

                    if (player == nullptr) return;

                    (void)player->GetMount(horse);

                    if (horse == nullptr)
                    {
                        SendNotification("No Mount Detected");
                        return;
                    }

                    if (!horse->IsMoving()) MoveHorse(MoveType::MoveJump);
                }
#pragma endregion

#pragma region Faster
                else if (currentCommand.Name == "horse - faster")
                {
                    RE::ActorPtr horse;

                    if (player == nullptr) return;

                    (void)player->GetMount(horse);

                    if (horse == nullptr)
                    {
                        SendNotification("No Mount Detected");
                        return;
                    }

                    if (IsActorWalking(horse))
                        MoveHorse(MoveType::MoveRun);

                    else if (horse->IsRunning())
                        MoveHorse(MoveType::MoveSprint);
                }
#pragma endregion

#pragma region Slower
                else if (currentCommand.Name == "horse - slower")
                {
                    RE::ActorPtr horse;

                    if (player == nullptr) return;

                    (void)player->GetMount(horse);

                    if (horse == nullptr)
                    {
                        SendNotification("No Mount Detected");
                        return;
                    }

                    if (IsActorWalking(horse))
                        MoveHorse(MoveType::StopMoving);

                    else if (horse->IsRunning())
                        MoveHorse(MoveType::MoveWalk);

                    else if (horse->AsActorState()->IsSprinting())
                        MoveHorse(MoveType::MoveRun);
                }
#pragma endregion

#pragma region Dismount
                else if (currentCommand.Name == "horse - dismount")
                {
                    // Stop the horse
                    MoveHorse(MoveType::StopMoving);

                    // Press "E" to Dismount
                    PressKey(ToSkyrimKeyCode("E"));
                }
#pragma endregion

#pragma region Backwards(Multipurpose)
                else if (currentCommand.Name == "horse - backwards")
                {
                    RE::ActorPtr horse;

                    if (player == nullptr) return;

                    (void)player->GetMount(horse);

                    if (horse == nullptr)
                    {
                        SendNotification("No Mount Detected");
                        return;
                    }

                    if (horse->IsMoving())
                        MoveHorse(MoveType::TurnAround);  // If the horse is moving, turn them around
                    else
                        MoveHorse(MoveType::MoveJump);  // If the horse is not moving, meaning it won't jump when "Space" is pressed, Rear the horse
                }
#pragma endregion
#pragma endregion

#pragma region Clear Hands /Voice
                else if (currentCommand.Name == "clear hands")
                {
                    logger::info("Clear Hands");
                    UnEquipFromActor(player, ActorSlot::Both);
                    SendNotification("Cleared Hands");
                }
                else if (currentCommand.Name == "clear shout")
                {
                    logger::info("Clear Voice");
                    UnEquipFromActor(player, ActorSlot::Voice);
                    SendNotification("Cleared Voice");
                }
#pragma endregion

#pragma region Save/Load Game
                else if (currentCommand.Name == "quick save")
                {
                    // Spoof button input to perform a quick save
                    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton())
                    {
                        auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "quicksave", 0, 1.0f, 0.0f);
                        bsInputEventQueue->PushOntoInputQueue(kEvent);
                    }
                }
                else if (currentCommand.Name == "quick load")
                {
                    switch ((int)VOX_QuickLoadType->value)
                    {
                        case 1:
                            // Loads most recent save file
                            SkyrimMessageBox::Show("Do you want to load your last save game?", {"Yes", "No"}, [](unsigned int result) {
                                switch (result)
                                {
                                    case 0:
                                        RE::BGSSaveLoadManager::GetSingleton()->LoadMostRecentSaveGame();
                                        // No notificaiton is needed, as the game already says "quicksaving..."
                                        break;

                                    case 1:
                                        SendNotification("Load Game Aborted");
                                        break;
                                }
                            });
                            break;

                        case 2:
                            RE::BGSSaveLoadManager::GetSingleton()->LoadMostRecentSaveGame();
                            // No notificaiton is needed, as the game already says "quicksaving..."
                            break;
                    }

                    /*
                    // Spoof button input to load most recent quick save file
                    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
                        auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "quickload", 0, 1.0f, 0.0f);
                        bsInputEventQueue->PushOntoInputQueue(kEvent);
                        SendNotification("Loading most recent quick save");
                    }
                    */
                }
#pragma endregion
            }

// Keybind
#pragma region Keybinds
            else if (currentCommand.Type == "keybind")
            {
                if (currentCommand.KeybindDuration >= 0)
                {
                    PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                    SendNotification("Keybind Press: " + std::to_string(currentCommand.ID));
                }
                else
                {
                    switch (currentCommand.KeybindDuration)
                    {
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
            }
#pragma endregion

// Console
#pragma region Console Commands
            else if (currentCommand.Type == "console")
            {
                try
                {
                    SendNotification("Console: " + currentCommand.Name);
                    std::string name = currentCommand.Name;
                    std::string line = "";
                    std::string count = "";
                    std::vector<std::string> list;

                    for (int i = 0; i < name.length(); i++)
                    {
                        for (line = ""; i < name.length() && name[i] != '+' && name[i] != '*'; i++)
                        {
                            line += name[i];
                        }

                        if (name[i] == '*')
                        {
                            for (i++, count = ""; i < name.length() && name[i] != '+'; i++)
                            {
                                count += name[i];
                            }

                            for (int j = std::stoi(count); j > 0; j--)
                            {
                                list.push_back(line);
                            }
                        }
                        else
                            list.push_back(line);

                    }  // End for

                    std::jthread console(ExecuteConsoleCommand, list);
                    console.detach();
                }
                catch (exception ex)
                {
                    SendNotification("ERROR: Invalid Command: " + currentCommand.Name);
                    logger::error("Invalid Console Command: {}\n{}", currentCommand.Name, ex.what());
                }
            }
#pragma endregion

// Locations
#pragma region Location and Map Marker
            else if (currentCommand.Type == "location")
            {
                if (currentCommand.Name == "playerlocation")
                    NavigateToPlayer();
                else
                    NavigateToLocation(currentCommand.Name);

                // Locations
            }
            else if (currentCommand.Type == "placemapmarker")
            {
                PlaceCustomMarker();

            }  // End type check
#pragma endregion
        });  // End Task
    }
}

#pragma region Event triggers for CheckUpdate()

#pragma region Animation Events

constexpr uint32_t exHash(const char* data, size_t const size) noexcept
{
    uint32_t hash = 5381;

    for (const char* c = data; c < data + size; ++c)
    {
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
    switch (exHash(name, eventName.size()))
    {
        case "weaponDraw"_h:
            logger::debug("Weapon has been drawn!!");
            break;

        case "weaponSheathe"_h:
            logger::debug("Weapon has been sheathed!!");
            break;

        case "MountEnd"_h:

            switch (PlayerMount())
            {
                case 1:
                    logger::debug("Player has mounted a Horse!!");
                    SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "mount\thorse");
                    break;

                case 2:
                    logger::debug("Player has mounted a Dragon!!");
                    SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "mount\tdragon");
                    break;
            }

            SendMessage(WebSocketMessage::InitializeUpdate);

            break;

        case "StopHorseCamera"_h:
            logger::debug("Player is now dismounted!!");
            SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "mount\tnone");
            SendMessage(WebSocketMessage::InitializeUpdate);
            // Stop all residual movement from horse controls
            SendKeyUp(ToSkyrimKeyCode("W"));  // Direction (N)
            SendKeyUp(ToSkyrimKeyCode("D"));  // Direction (E)
            SendKeyUp(ToSkyrimKeyCode("S"));  // Direction (S)
            SendKeyUp(ToSkyrimKeyCode("A"));  // Direction (W)

            SendKeyUp(ToSkyrimKeyCode("LAlt"));  // LAlt (Toggles walking/running)
            break;

        default:
            return;
    }
    if (readyForUpdate == false)
    {  // Check if CheckUpdate should NOT fully execute
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
    // Werewolf
    if (IsPlayerWerewolf())
    {
        logger::debug("Player Morphed into Werewolf");
        SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "morph\twerewolf");
    }

    // Vampire Lord
    else if (IsPlayerVampireLord())
    {
        logger::debug("Player Morphed into Vampire Lord");
        SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "morph\tvampirelord");
    }

    // "Normal" player morph
    else
    {
        logger::debug("Player Reverted their Morph");
        SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "morph\tnone");
    }

    SendMessage(WebSocketMessage::InitializeUpdate);

    std::this_thread::sleep_for(std::chrono::seconds(1));  // Delay update one second to allow everything to update. This prevents stacking updates on C#
    CheckUpdate();                                         // Call method to check for game data updates
}

// Executes when game loads
void LoadGameEvent::EventHandler::GameLoaded()
{
    logger::debug("Game loaded!!");
    switch (PlayerMount())
    {
        case 0:
            SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "mount\tnone");
            break;

        case 1:
            logger::debug("Player started on a Horse!!");
            SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "mount\thorse");
            break;

        case 2:
            logger::debug("Player started on a Dragon!!");
            SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "mount\tdragon");
            break;
    }

    switch (PlayerMorph())
    {
        case 0:
            SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "morph\tnone");
            break;

        case 1:
            logger::debug("Player started as a werewolf!!");
            SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "morph\twerewolf");
            break;

        case 2:
            logger::debug("Player started as a Vampire Lord!!");
            SendMessage(WebSocketMessage::UpdateConfiguration + (std::string) "morph\tvampirelord");
            break;
    }
    CheckUpdate();  // Call method to check for game data updates
}

// Executes when a menu opens or closes
void MenuOpenCloseEvent::EventHandler::MenuOpenClose(const RE::MenuOpenCloseEvent* event)
{
    string menuName = event->menuName.c_str();  // Capture name of triggering menu
    ///logger::info("Trigger Menu = {}!!", menuName);
    MenuType type;
    static std::unordered_map<std::string, MenuType> const table = {
        {"Console", MenuType::Console},      {"FavoritesMenu", MenuType::Favorites}, {"InventoryMenu", MenuType::Inventory},
        {"Journal Menu", MenuType::Journal}, {"LevelUp Menu", MenuType::LevelUp},    {"MagicMenu", MenuType::Magic},
        {"MapMenu", MenuType::Map},          {"StatsMenu", MenuType::Skills},        {"Sleep/Wait Menu", MenuType::SleepWait},
        {"TweenMenu", MenuType::Tween}};
    auto it = table.find(menuName);
    if (it != table.end())  // Check if match was found within enums
        type = it->second;
    else  // No enum match found
        return;

    //switch (type)
    //{  // Check if triggering menu is of interest
    //    case MenuType::Console:
    //        menuName = RE::Console::MENU_NAME;
    //        break;

    //    case MenuType::Favorites:
    //        menuName = RE::FavoritesMenu::MENU_NAME;
    //        break;

    //    case MenuType::Inventory:
    //        menuName = RE::InventoryMenu::MENU_NAME;
    //        break;

    //    case MenuType::Journal:
    //        menuName = RE::JournalMenu::MENU_NAME;
    //        break;

    //    case MenuType::LevelUp:
    //        menuName = RE::LevelUpMenu::MENU_NAME;
    //        break;

    //    case MenuType::Magic:
    //        menuName = RE::MagicMenu::MENU_NAME;
    //        break;

    //    case MenuType::Map:
    //        menuName = RE::MapMenu::MENU_NAME;
    //        if (event->opening)
    //            SendMessage(WebSocketMessage::EnableLocationCommands);
    //        else
    //            SendMessage(WebSocketMessage::DisableLocationCommands);
    //        break;

    //    case MenuType::Skills:
    //        menuName = RE::StatsMenu::MENU_NAME;
    //        break;

    //    case MenuType::SleepWait:
    //        menuName = RE::SleepWaitMenu::MENU_NAME;
    //        break;

    //    case MenuType::Tween:
    //        menuName = RE::TweenMenu::MENU_NAME;
    //        break;

    //    default:  // Triggering menu is not of interest
    //        logger::error("Error processing menu event - unexpected enum encountered");
    //        return;
    //}
    if (menuName == RE::MapMenu::MENU_NAME)
    {
        if (event->opening)
            SendMessage(WebSocketMessage::EnableLocationCommands);
        else
            SendMessage(WebSocketMessage::DisableLocationCommands);
    }
    if (event->opening == true)
    {  // Check if captured event involves menu OPENING
        openMenu = menuName;
        ///SendNotification(" OPEN Menu = " + openMenu);
        ///logger::info("OPEN Menu = {}!!", menuName);
    }
    else
    {  // Captured event involves menu CLOSING
        openMenu = "";
        ///logger::debug("CLOSED Menu = {}!!", menuName);
    }
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

//// Executes when a menu opens or closes
//void MenuOpenCloseEvent::EventHandler::MenuOpenClose(const RE::MenuOpenCloseEvent* event)
//{
//    string menuName = event->menuName.c_str();  // Capture name of triggering menu
//    logger::info("Trigger Menu = {}!!", menuName);
//    MenuType type;
//    static std::unordered_map<std::string, MenuType> const table = {
//        {"Console", MenuType::Console},      {"FavoritesMenu", MenuType::Favorites}, {"InventoryMenu", MenuType::Inventory},
//        {"Journal Menu", MenuType::Journal}, {"LevelUp Menu", MenuType::LevelUp},    {"MagicMenu", MenuType::Magic},
//        {"MapMenu", MenuType::Map},          {"StatsMenu", MenuType::Skills},        {"Sleep/Wait Menu", MenuType::SleepWait},
//        {"TweenMenu", MenuType::Tween}};
//    auto it = table.find(menuName);
//    if (it != table.end())  // Check if match was found within enums
//        type = it->second;
//    else  // No enum match found
//        return;
//    switch (type)
//    {  // Check if triggering menu is of interest
//        case MenuType::Console:
//            menuName = RE::Console::MENU_NAME;
//            break;
//
//        case MenuType::Favorites:
//            menuName = RE::FavoritesMenu::MENU_NAME;
//            break;
//
//        case MenuType::Inventory:
//            menuName = RE::InventoryMenu::MENU_NAME;
//            break;
//
//        case MenuType::Journal:
//            menuName = RE::JournalMenu::MENU_NAME;
//            break;
//
//        case MenuType::LevelUp:
//            menuName = RE::LevelUpMenu::MENU_NAME;
//            break;
//
//        case MenuType::Magic:
//            menuName = RE::MagicMenu::MENU_NAME;
//            break;
//
//        case MenuType::Map:
//            menuName = RE::MapMenu::MENU_NAME;
//            if (event->opening)
//                SendMessage(WebSocketMessage::EnableLocationCommands);
//            else
//                SendMessage(WebSocketMessage::DisableLocationCommands);
//            break;
//
//        case MenuType::Skills:
//            menuName = RE::StatsMenu::MENU_NAME;
//            break;
//
//        case MenuType::SleepWait:
//            menuName = RE::SleepWaitMenu::MENU_NAME;
//            break;
//
//        case MenuType::Tween:
//            menuName = RE::TweenMenu::MENU_NAME;
//            break;
//
//        default:  // Triggering menu is not of interest
//            logger::error("Error processing menu event - unexpected enum encountered");
//            return;
//    }
//    if (event->opening == true)
//    {  // Check if captured event involves menu OPENING
//        openMenu = menuName;
//        logger::info("OPEN Menu = {}!!", menuName);
//    }
//    else
//    {  // Captured event involves menu CLOSING
//        openMenu = "";
//        logger::debug("CLOSED Menu = {}!!", menuName);
//    }
//    CheckUpdate();  // Call method to check for game data updates
//
//    // string menuName = event->menuName.c_str();  // Capture name of closed menu
//    // if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if trigger menu is of interest
//    //     logger::info("Trigger Menu = {}!!", menuName);
//    //     if (event->opening == true) // Check if captured event involves menu OPENING
//    //         logger::debug("OPENED Menu = {}!!", menuName);
//    //     else
//    //         logger::debug("CLOSED Menu = {}!!", menuName);
//    //     CheckUpdate();  // Call method to check for game data updates
//    // }
//
//    ///// logger::debug("Menu opened or closed!!");
//    // if (event->opening == false) {  // Check if capture event involves menu CLOSING
//    //     string menuName = event->menuName.c_str(); // Capture name of closed menu
//    //     /// logger::info("CLOSE Menu = {}!!", menuName);
//    //     if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if menu of interest was closed
//    //         logger::debug("CLOSED Menu = {}!!", menuName);
//    //         CheckUpdate(); // Call method to check for game data updates
//    //     }
//    // } else {
//    //     string menuName = event->menuName.c_str();  // Capture name of opened menu
//    //     /// logger::info("OPEN Menu = {}!!", menuName);
//    //     if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if menu of interest was closed
//    //         logger::debug("CLOSED Menu = {}!!", menuName);
//    //         CheckUpdate();  // Call method to check for game data updates
//    //     }
//    // }
//}

// Executes when new locations are discovered
void LocationDiscoveredEvent::EventHandler::LocationDiscovered(string locationName)
{
    logger::debug("{} discovered", locationName);
    CheckUpdate();
}

#pragma endregion All tracked game events that trigger an UpdateCheck

#pragma region Device Input Processing
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

    if (VOX_PushToSpeakType->value == 0 || VOX_PushToSpeakType->value == 3 ||
        keyCode != VOX_PushToSpeakKeyCode->value)  // Check if PushToSpeak is disabled OR triggering keyCode does NOT match target input value from VOX MCM
        return;                                    // Exit this method

    if (VOX_PushToSpeakType->value == 1)
    {                        // Button must be Held
        thread([button]() {  // Create new thread for execution (passing in button)
            // SendNotification("Flatrim Input Triggered - Start listening!");
            SendMessage(WebSocketMessage::EnableRecognition);
            while (button->IsPressed()) Sleep(250);  // Pause while input trigger is still being pressed
            // SendNotification("Flatrim Input released - Stop Listening!");
            SendMessage(WebSocketMessage::DisableRecognition);
        })
            .detach();
    }
    else if (VOX_PushToSpeakType->value == 2)
    {                        // Button toggles
        thread([button]() {  // Create new thread for execution (passing in button)
            if (!pushToSpeakListening)
            {                                 // Check if recognition app is NOT listening
                pushToSpeakListening = true;  // Set isRecognitionEnabled flag
                // SendNotification("Flatrim Input Triggered - Start listening!");
                SendMessage(WebSocketMessage::EnableRecognition);
            }
            else
            {
                pushToSpeakListening = false;  // Reset isRecognitionEnabled flag
                // SendNotification("Flatrim Input released - Stop Listening!");
                SendMessage(WebSocketMessage::DisableRecognition);
            }
            while (button->IsPressed()) Sleep(250);  // Pause while input trigger is still being pressed
        })
            .detach();
    }
}

// Legacy API event handlers
void OnVRButtonEvent(PapyrusVR::VREventType type, PapyrusVR::EVRButtonId buttonId, PapyrusVR::VRDevice deviceId)
{
    /*
    VOX_PushToSpeakType:
        [0] = Disabled
        [1] = Hold
        [2] = Press (Toggle)
        [3] = Vocal
    */

    std::string notification;

    switch (type)
    {
        case PapyrusVR::VREventType_Pressed:
            notification = "VR Button Pressed deviceId: " + std::to_string(deviceId) + " buttonId: " + std::to_string(buttonId);
            break;

        case PapyrusVR::VREventType_Released:
            notification = "VR Button Released deviceId: " + std::to_string(deviceId) + " buttonId: " + std::to_string(buttonId);
            break;

        case PapyrusVR::VREventType_Touched:
            notification = "VR Button Touched deviceId: " + std::to_string(deviceId) + " buttonId: " + std::to_string(buttonId);
            break;

        case PapyrusVR::VREventType_Untouched:
            notification = "VR Button Untouched deviceId: " + std::to_string(deviceId) + " buttonId: " + std::to_string(buttonId);
            break;
    }

    // Check if PushToSpeak is disabled OR triggering buttonId does NOT match target input value from VOX MCM
    if (VOX_PushToSpeakType->value == 0 || VOX_PushToSpeakType->value == 3 ||
        (VOX_PushToSpeakKeyCode->value > 474 && buttonId + 474 != VOX_PushToSpeakKeyCode->value) ||
        (VOX_PushToSpeakKeyCode->value <= 474 && buttonId + 410 != VOX_PushToSpeakKeyCode->value))
        return;  // Exit this method

    if (VOX_PushToSpeakType->value == 1)  // VOX_PushToSpeakType mode requires trigger to be held
    {
        if (type == PapyrusVR::VREventType_Pressed)
        {
            logger::debug("Push-To-Speak Keycode Recognized - Pressed");
            if (pushToSpeakListening == false)
            {
                pushToSpeakListening = true;
                SendMessage(WebSocketMessage::EnableRecognition);
                // SendNotification("VR Input Pressed - Start Listening!");
            }
        }
        else if (type == PapyrusVR::VREventType_Released)
        {
            logger::debug("Push-To-Speak Keycode Recognized - Released");
            if (pushToSpeakListening == true)
            {
                pushToSpeakListening = false;
                SendMessage(WebSocketMessage::DisableRecognition);
                // SendNotification("VR Input Released - Stop Listening!");
            }
        }
    }
    else if (VOX_PushToSpeakType->value == 2)  // VOX_PushToSpeakType mode has trigger acting as a toggle
    {
        if (type == PapyrusVR::VREventType_Released)
        {
            logger::debug("Push-To-Speak Keycode Recognized - Toggled");
            if (pushToSpeakListening == false)
            {
                SendMessage(WebSocketMessage::EnableRecognition);
                // SendNotification("VR Input Released - Start Listening!");
            }
            else
            {
                SendMessage(WebSocketMessage::DisableRecognition);
                // SendNotification("VR Input Released - Stop Listening!");
            }
            pushToSpeakListening = !pushToSpeakListening;
        }
    }
}

#pragma endregion device input triggers for speech recognition listening

// Initializes Plugin, Speech Recognition, Websocket, and data tracker
SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    try
    {
        SKSE::Init(skse);  // Initialize SKSE plugin
        ///MessageBoxA(NULL, "Press OK when you're ready!", "Skyrim", MB_OK | MB_ICONQUESTION);  // MessageBox to halt execution so a debugger can be attached
        SetupLog();  // Set up the debug logger

        SKSE::GetMessagingInterface()->RegisterListener(
            OnMessage);  // Listen for game messages (usually "Is Game Loaded" comes first) and execute OnMessage method in response
        return true;
    }
    catch (exception ex)
    {
        logger::info("ERROR initializing Voxima SKSE plugin: {}", ex.what());
        return false;
    }
}  // End SKSEPluginLoad

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