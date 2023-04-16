// TODO
// figure out how to more "silently" launch the speech reco app (avoid showing extraneous windows for a brief second)
// add final message from Skyrim to gracefully close speech reco app?
// is the content related to PlayerInfo.txt still relevant/accurate?
// disposition GameLoaded vs. kPostLoadGame vs. Is3DLoaded() looping for getting FIRST full game and app update (first two options don't trigger at beginning of Alternate Start)

#include <windows.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <ShellAPI.h>
#include <filesystem>
#include <format>
#include <string>

#include "functions.hpp"  // Miscellaneous custom functions
#include "websocket.hpp" // Websocket functionality
#include "animation-events.hpp" // Animation event hooking and processing
#include "spell-learned-event.hpp" // Spell learn event hooking and processing
#include "morph-changed-event.hpp" // Player morph event hooking and processing
#include "load-game-event.hpp" // Game load event hooking and processing
#include "menu-close-event.hpp" // Menu close event hooking and processing
#include "location-discovery-event.hpp" // Location discovery event hooking and processing
#include "device-input-event.hpp"

struct Command {
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

void TestMethod();

void InitialUpdate();
void CheckUpdate(bool loop = false, bool isAsync = false);
void Update(std::string update = "");
void ExecuteCommand(Command command);

float currentVocalPTS;
float currentSensitivity;
float currentAutoCastShouts;
float currentAutoCastPowers;

std::string knownShouts = "";
std::string currentShouts = "";
std::string knownCommand = "";
std::vector<std::string> knownLocations;
std::vector<std::string> currentLocations;

int updateQueue = 0;
std::string id = "";
int numMagic[] = {0, 0};

int currentMorph = Morph::Player;

bool saveTriggered = false;
bool readyForUpdate = true;

// Method executed when SKSE messages are received
void OnMessage(SKSE::MessagingInterface::Message* message) {
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
                InitializeDeviceInputHooking();        // Setup device input event monitoring
                
                while (connected == false)                                        // Loop while websocket connection has not been made
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));  // Brief pause to allow for websocket connection to be made
                InitialUpdate();                                                  // Run initial data update
                break;

            //// Save game has been loaded
            //case SKSE::MessagingInterface::kPostLoadGame:
            //    logger::debug("Saved game loaded!!");
            //    CheckUpdate();
            //    break;

            // Save Game as been created (manual or autosave)
            case SKSE::MessagingInterface::kSaveGame:
                if (saveTriggered == false) {
                    saveTriggered = true;
                    logger::debug("Save game created!!");
                    CheckUpdate();
                }
                break;
        }//End switch

    } catch (const std::exception& ex) {
        logger::error("ERROR during OnMessage: {}", ex.what());
    }
}

// Initial game data gathering
void InitialUpdate() {
    try {
        // logger::info("{}", "Path is " + SKSE::log::log_directory().value().string());
        // SendMessage("Path is " + SKSE::log::log_directory().value().string());

        // Start Mod
        logger::info("Starting Mod");

        player = RE::PlayerCharacter::GetSingleton();

        VOX_Enabled = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_Enabled");
        VOX_UpdateInterval = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_UpdateInterval");
        VOX_CheckForUpdate = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_CheckForUpdate");
        VOX_PushToSpeak = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_PushToSpeak");
        VOX_ShoutKey = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_ShoutKey");
        VOX_ShowLog = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_ShowLog");
        VOX_LongAutoCast = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_LongAutoCast");
        VOX_AutoCastPowers = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_AutoCastPowers");
        VOX_AutoCastShouts = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_AutoCastShouts");

        // C++ --> C# Variables
        VOX_VocalPushToSpeak = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_VocalPushToSpeak");
        VOX_Sensitivity = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_Sensitivity");
        currentVocalPTS = VOX_VocalPushToSpeak->value;
        currentSensitivity = VOX_Sensitivity->value;
        currentAutoCastShouts = VOX_AutoCastShouts->value;
        currentAutoCastPowers = VOX_AutoCastPowers->value;

    } catch (const std::exception& ex) {
        logger::error("ERROR during InitialUpdate: {}", ex.what());
    }
}

// Checks for changes in the tracked data
void CheckUpdate(bool loop, bool isAsync) {
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

        ///*** this is for later to make sure everything is ready to go once game loads and looping is replaced by events
        /* while (true) { // Loop ~indefinitely
                if (!player->Is3DLoaded()) // Check if game 3D content is loaded (player character shown)
                    std::this_thread::sleep_for(std::chrono::seconds(1));  // 1 second pause
                else
                    break; // Break out of parent "while" loop
        } */

        if (loop) logger::debug("CheckUpdate loop start");

        do {
            if (!player->Is3DLoaded()) {
                if (loop) std::this_thread::sleep_for(std::chrono::seconds(1));  // 1 second delay to avoid looping unnecessarily
                continue;

            } else if (VOX_Enabled->value == 0) {
                if (loop)
                    std::this_thread::sleep_for(std::chrono::seconds(5));  // 5 second delay to not loop unnecessarily. See Bottom of page Note (1)
                continue;
            }

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
            if (isPlayerWerewolf()) {  // If Werewolf
                if (currentMorph != Morph::Werewolf) {
                    currentMorph = Morph::Werewolf;
                    logger::info("Sending \"Werewolf\" Morph to C#");
                    std::this_thread::sleep_for(
                        std::chrono::seconds(1));  // Delay update one second to allow everything to update. This prevents stacking updates on C#
                    fileUpdate = true;
                }
                currentMorph = Morph::Werewolf;  ///*** is this needed?

                // Vampire Lord
            } else if (isPlayerVampireLord()) {  // If Vampire Lord
                if (currentMorph != Morph::VampireLord) {
                    currentMorph = Morph::VampireLord;
                    logger::info("Sending \"Vampire Lord\" Morph to C#");
                    std::this_thread::sleep_for(
                        std::chrono::seconds(1));  // Delay update one second to allow everything to update. This prevents stacking updates on C#
                    fullUpdate = true;
                }
                currentMorph = Morph::VampireLord;  ///*** is this needed?

                // "Normal" player morph
            } else if (currentMorph != Morph::Player) {  // If not a morph but marked as one
                currentMorph = Morph::Player;
                logger::info("Sending \"Reverting Morph\" to C#");
                std::this_thread::sleep_for(
                    std::chrono::seconds(1));  // Delay update one second to allow everything to update. This prevents stacking updates on C#
                fullUpdate = true;
            }

            //-----Current Mount-----//
            switch (playerMount()) {
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
            if (currentVocalPTS != VOX_VocalPushToSpeak->value) {
                currentVocalPTS = VOX_VocalPushToSpeak->value;
                logger::info("Sending \"VOX_VocalPushToSpeak = {}\" to C#", VOX_VocalPushToSpeak->value);
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
                VOX_CheckForUpdate->value = 0;
                logger::info("Forcing a Full Update");
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
            if (currentLocations != knownLocations){
                currentLocations = knownLocations;
                std::string locationsMessage = "";
                std::string currentMarker;

                for (auto playerMapMarker : currentLocations) {
                    locationsMessage += playerMapMarker + "\n";
                }
                logger::info("{}", locationsMessage);

                SendMessage("update locations\n" + locationsMessage + "playerlocation");
            }

            if (fullUpdate)
                Update();
            else if (fileUpdate)
                Update("file");

#pragma endregion

            if (loop) std::this_thread::sleep_for(std::chrono::seconds((int)VOX_UpdateInterval->value));  // Pause based on VOX MCM settings
        } while (loop);                                                                                   // End While

    } catch (const std::exception& ex) {
        logger::error("ERROR during CheckUpdate: {}", ex.what());
        if (loop)
            CheckUpdate(true);  // Restart this method so that the mod can continue working
        else
            CheckUpdate();
    }// End Try/Catch
}  // End CheckUpdate

// Gets and sends game data to C#
void Update(std::string update) {
    try {
        logger::info("Updating Information");
        bool finished = false;
        std::string type = update;
        std::string updateFile = "";
        std::string updateSpells = "";
        std::string updatePowers = "";
        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });  // sets type.ToLower()

        if (type == "werewolf") // Morph
            finished = true;
        else if (type == "file") {
            logger::info("Updating C++ -> C# File (PlayerInfo.txt)");
            finished = true;
        } else if (type == "")
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

        if (VOX_VocalPushToSpeak->value == 1)
            updateFile += "VOX_VocalPushToSpeak\ttrue\n";
        else
            updateFile += "VOX_VocalPushToSpeak\tfalse\n";

        if (VOX_AutoCastShouts->value == 1)
            updateFile += "VOX_AutoCastShouts\ttrue\n";
        else
            updateFile += "VOX_AutoCastShouts\tfalse\n";

        if (VOX_AutoCastPowers->value == 1)
            updateFile += "VOX_AutoCastPowers\ttrue\n";
        else
            updateFile += "VOX_AutoCastPowers\tfalse\n";

        updateFile += "VOX_Sensitivity\t" + std::to_string((int)VOX_Sensitivity->value);

        SendMessage("update configuration\n" + updateFile);

        if (!finished) {
            auto magic = GetActorMagic(player, MagicType::Spell, MagicType::Power);

            SendMessage("update spells\n" + magic[0]);
            SendMessage("update powers\n" + magic[1]);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Brief pause to ensure the following message is sent and processed after the previous messages
        SendMessage("initialize update");

    } catch (const std::exception& ex) {
        logger::error("ERROR during Update:\n{}\n", ex.what());
    }
}

/*
// Process messages received from websocket server
void ProcessReceivedMessage(const string& command) {
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

        if (VOX_PushToSpeak->value != -1 && !IsKeyDown(VOX_PushToSpeak->value)) {
            logger::info("Received message but rejected due to Push-To-Speak button not being held: \"{}\"", messagePrint);
            return;
        }

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
                case 0:  // Name
                    if (line == "") continue;
                    currentCommand.Name = line;
                    break;

                case 1:  // ID
                    if (line == "") continue;
                    currentCommand.ID = std::stoi(line);
                    break;

                case 2:  // ???
                    if (line == "") continue;
                    currentCommand.KeybindDuration = std::stoi(line);
                    break;

                case 3:  // Type
                    if (line == "") continue;
                    currentCommand.Type = line;
                    break;

                case 4:  // Source File
                    if (line == "") continue;
                    currentCommand.fileName = line;
                    break;

                case 5:  // ???
                    if (line == "") continue;
                    currentCommand.Hand = std::stoi(line);
                    break;

                case 6:  // Autocast boolean
                    if (line == "") continue;
                    currentCommand.AutoCast = std::stoi(line);
                    break;

                case 7:  // Morph
                    if (line == "") continue;
                    currentCommand.Morph = line;
                    break;
            }  // End switch
        }      // End for

        switch (playerMorph()) {
            case 0:
                if (currentCommand.Morph != "none") {
                    logger::info("Command Ignored. Mismatched Morph. (Current: none) (Command: {})", currentCommand.Morph);
                    return;
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
        if (currentCommand.Type != "keybind") RE::DebugNotification(currentCommand.Name.c_str());
    } catch (exception ex) {
        logger::error("ERROR while preprocessing\n\"{}\"\nmessage from server: \"{}\"", command, ex.what());
    }
} */

// Process messages received from websocket server
void ProcessReceivedMessage(const string& command) {
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

        if (VOX_PushToSpeak->value != -1 && !IsKeyDown(VOX_PushToSpeak->value)) {
            logger::info("Received message but rejected due to Push-To-Speak button not being held: \"{}\"", messagePrint);
            return;
        }

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

        switch (playerMorph()) {
            case 0:
                if (currentCommand.Morph != "none") {
                    switch (playerMount()) {
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
        if (currentCommand.Type != "keybind") RE::DebugNotification(currentCommand.Name.c_str());
    } catch (exception ex) {
        logger::error("ERROR while preprocessing\n\"{}\"\nmessage from server: \"{}\"", command, ex.what());
    }
}

// Executes a given command
void ExecuteCommand(Command command) {
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

                logger::info("Spell Command Identified: Name: \"{}\"     Hand: \"{}\"", item->GetName(), currentCommand.Hand);

                switch (currentCommand.Hand) {
                    case 0:
                        if (!(currentCommand.AutoCast && item->As<RE::MagicItem>()->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget &&
                              (item->As<RE::SpellItem>()->data.chargeTime <= 1 || VOX_LongAutoCast->value == 1) && CastMagic(player, item, ActorSlot::Left))) {
                            EquipToActor(player, item, ActorSlot::Left);
                        }
                        break;

                    case 1:
                        if (!(currentCommand.AutoCast && item->As<RE::MagicItem>()->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget &&
                              (item->As<RE::SpellItem>()->data.chargeTime <= 1 || VOX_LongAutoCast->value == 1) && CastMagic(player, item, ActorSlot::Right))) {
                            EquipToActor(player, item, ActorSlot::Right);
                        }
                        break;

                    case 2:
                        if (!(currentCommand.AutoCast && item->As<RE::MagicItem>()->GetCastingType() == RE::MagicSystem::CastingType::kFireAndForget &&
                              (item->As<RE::SpellItem>()->data.chargeTime <= 1 || VOX_LongAutoCast->value == 1) && CastMagic(player, item, ActorSlot::Both))) {
                            EquipToActor(player, item, ActorSlot::Both);
                        }
                        break;
                }  // End Switch

                // Clear Hands
            } else if (currentCommand.Type == "clear hands") {
                logger::info("Clear Hands");
                UnEquipFromActor(player, ActorSlot::Both);

                // Clear Voice
            } else if (currentCommand.Type == "clear shout") {
                logger::info("Clear Voice");
                UnEquipFromActor(player, ActorSlot::Voice);

                // Power
            } else if (currentCommand.Type == "power") {
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

                // Shout
            } else if (currentCommand.Type == "shout") {
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
            } else if (currentCommand.Type == "keybind") {
                if (currentCommand.Morph != "horseriding") {
                    if (currentCommand.KeybindDuration >= 0)
                        PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                    else
                        switch (currentCommand.KeybindDuration) {
                            case -1:
                                SendKeyDown(currentCommand.ID);
                                break;
                            case -2:
                                SendKeyUp(currentCommand.ID);
                                break;
                            default:
                                PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                                break;
                        }
                } else {
                    // Command is likely meant for a horse
                    RE::BSInputDeviceManager* gamepad = RE::BSInputDeviceManager::GetSingleton();
                    gamepad->GetGamepadHandler()->Initialize();

                    switch (currentCommand.ID) {
                        // W
                        case 17:
                            if (currentCommand.KeybindDuration == -1)
                                MoveHorse(MoveType::MoveForward);
                            else
                                MoveHorse(MoveType::StopMoving);
                            break;

                            // A
                        case 30:
                            if (currentCommand.KeybindDuration == 0)
                                MoveHorse(MoveType::MoveLeft);
                            else
                                MoveHorse(MoveType::TurnLeft);

                            break;

                            // S
                        case 31:
                            MoveHorse(MoveType::MoveBackward);
                            break;

                            // D
                        case 32:
                            if (currentCommand.KeybindDuration == 0)
                                MoveHorse(MoveType::MoveRight);
                            else
                                MoveHorse(MoveType::TurnRight);

                            break;

                            // Space
                        case 57:
                            MoveHorse(MoveType::MoveJump);
                            break;

                            // Shift
                        case 42:
                            if (currentCommand.KeybindDuration == 0)
                                MoveHorse(MoveType::MoveSprint);
                            else
                                MoveHorse(MoveType::StopSprint);
                            break;

                        default:  // Not a command for controlling the horse
                            if (currentCommand.KeybindDuration >= 0)
                                PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                            else
                                switch (currentCommand.KeybindDuration) {
                                    case -1:
                                        SendKeyDown(currentCommand.ID);
                                        break;
                                    case -2:
                                        SendKeyUp(currentCommand.ID);
                                        break;
                                    default:
                                        PressKey(currentCommand.ID, currentCommand.KeybindDuration);
                                        break;
                                }
                    }
                }

            } else if (currentCommand.Type == "console") {
                try {
                    ///*** development only
                    if (currentCommand.Name == "this is test one") {
                        thread([]() { TestMethod(); }).detach();
                        return;
                    }

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

                        } else
                            list.push_back(line);

                    }  // End for

                    std::jthread console(ExecuteConsoleCommand, list);
                    console.detach();

                } catch (exception ex) {
                    logger::error("Invalid Console Command: {}\n{}", currentCommand.Name, ex.what());
                }

            } else if (currentCommand.Type == "location") {
                if (currentCommand.Name == "playerlocation")
                    NavigateToPlayer();
                else
                    NavigateToLocation(currentCommand.Name);

            }  // End type check
        });    // End Task
    }
}

void TestMethod() { 

    // Test auto-navigation to a known location on the map
    string targetLocation = "Winterhold";
    NavigateToLocation(targetLocation);

    //// Test auto-navigation to the player's current position on the map
    //NavigateToPlayer();

    //// Test auto-navigation to player's custom map marker
    //NavigateToCustomMarker();

}

#pragma region Event triggers for CheckUpdate()

#pragma region Animation Events

constexpr uint32_t exHash(const char* data, size_t const size) noexcept {
    uint32_t hash = 5381;

    for (const char* c = data; c < data + size; ++c) {
        hash = ((hash << 5) + hash) + (unsigned char)*c;
    }

    return hash;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept { return exHash(str, size); }

// Executes when animation events occur
void Anim::Events::AnimationEvent(const char* holder, const char* name) {

#pragma region Events of Interest

    // weaponDraw = take out weapon, spells, etc.
    // weaponSheathe = put away weapon, spells, etc.
    // MountEnd = occurs near beginning of mount horse or dragon
    // StopHorseCamera = occurs near the beginning of unmount horse or dragon

#pragma endregion Relevant events to track for UpdateCheck triggering    

    string eventName(name); // Convert passed-in argument to string
    ///logger::debug("{}: {}", holder, eventName);  // Output all the captured events to the log
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
void SpellLearnedEvent::EventHandler::SpellLearned(const RE::SpellItem* const spell) {
    logger::debug("New spell learned!!");
    ///logger::info("Spell Learned = {}", spell->fullName.c_str());
    ///logger::info("Spell Learned = {}", spell->GetName());
    ///CheckUpdate();  // Call method to check for game data updates
}

// Executes when player's morph changes
void MorphEvents::EventHandler::MorphChanged() {
    logger::debug("Player morph changed!!");
    CheckUpdate();  // Call method to check for game data updates
}

// Executes when game loads
void LoadGameEvent::EventHandler::GameLoaded() {
    logger::debug("Game loaded!!");
    CheckUpdate();  // Call method to check for game data updates
}

// Executes when a menu opens or closes
void MenuOpenCloseEvent::EventHandler::MenuOpenClose(const RE::MenuOpenCloseEvent* event) {
    string menuName = event->menuName.c_str(); // Capture name of triggering menu
    ///logger::info("Trigger Menu = {}!!", menuName);
    MenuType type;
    static std::unordered_map<std::string, MenuType> const table = {
        {"Console", MenuType::Console}, {"FavoritesMenu", MenuType::Favorites}, {"InventoryMenu", MenuType::Inventory}, 
        {"Journal Menu", MenuType::Journal}, {"LevelUp Menu", MenuType::LevelUp}, {"MagicMenu", MenuType::Magic},
        {"MapMenu", MenuType::Map}, {"StatsMenu", MenuType::Skills}, {"Sleep/Wait Menu", MenuType::SleepWait}, {"TweenMenu", MenuType::Tween}, 
    };
    auto it = table.find(menuName);
    if (it != table.end()) // Check if match was found within enums
        type = it->second;
    else // No enum match found
        return;
    switch (type) { // Check if triggering menu is of interest
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

        default: // Triggering menu is not of interest
            logger::error("Error processing menu event - unexpected enum encountered");
            return;
    }
    //if (event->opening == true) {  // Check if captured event involves menu OPENING
    //    logger::info("OPEN Menu = {}!!", menuName);
    //} else {  // Captured event involves menu CLOSING
    //    logger::debug("CLOSED Menu = {}!!", menuName);
    //}
    CheckUpdate();  // Call method to check for game data updates
    
    //string menuName = event->menuName.c_str();  // Capture name of closed menu
    //if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if trigger menu is of interest
    //    logger::info("Trigger Menu = {}!!", menuName);
    //    if (event->opening == true) // Check if captured event involves menu OPENING
    //        logger::debug("OPENED Menu = {}!!", menuName);
    //    else
    //        logger::debug("CLOSED Menu = {}!!", menuName);
    //    CheckUpdate();  // Call method to check for game data updates
    //}

    ///// logger::debug("Menu opened or closed!!");
    //if (event->opening == false) {  // Check if capture event involves menu CLOSING
    //    string menuName = event->menuName.c_str(); // Capture name of closed menu
    //    /// logger::info("CLOSE Menu = {}!!", menuName);
    //    if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if menu of interest was closed
    //        logger::debug("CLOSED Menu = {}!!", menuName);
    //        CheckUpdate(); // Call method to check for game data updates
    //    }
    //} else {
    //    string menuName = event->menuName.c_str();  // Capture name of opened menu
    //    /// logger::info("OPEN Menu = {}!!", menuName);
    //    if (menuName == "TweenMenu" || menuName == "Console" || menuName == "Journal Menu") {  // Check if menu of interest was closed
    //        logger::debug("CLOSED Menu = {}!!", menuName);
    //        CheckUpdate();  // Call method to check for game data updates
    //    }
    //}
}

// Executes when new locations are discovered
void LocationDiscoveredEvent::EventHandler::LocationDiscovered(string locationName) {
    logger::debug("{} discovered", locationName);
    CheckUpdate();
    

    ///*** do something with locationName or call GetKnownLocations()

    // do other stuff
}

#pragma endregion All tracked game events that trigger an UpdateCheck

#pragma region Device Input Processing

bool pushToTalk = true;
bool isListening = false;

// Executes when input device events are received
void DeviceInputEvent::DeviceInputHandler::InputDeviceEvent(RE::ButtonEvent* button, uint32_t keyCode) {
    /*
        EXAMPLE "KEY" TRIGGERS
        Keyboard backslash (\) = 43
        Mouse middle button = 258
        Gamepad right shoulder button = 275
    */

    if (keyCode != DBU_PushToSpeak->value) return;  // Check if triggering keyCode does NOT match target input value from DBU MCM, and if so exit this method

    if (pushToTalk) {        // Check if push-to-talk mode is enabled (true)
        thread([button]() {  // Create new thread for execution (passing in button)
            RE::DebugNotification("Input Triggered - Start listening!");

            /// *** do stuff to activate listening of C# app

            while (button->IsPressed()) Sleep(250);  // Pause while input trigger is still being pressed
            RE::DebugNotification("Input released - Stop Listening!");

            /// *** do stuff to deactivate listening of C# app
        })
            .detach();
    } else {
        thread([button]() {              // Create new thread for execution (passing in button)
            if (isListening == false) {  // Check if recognition app is NOT listening
                isListening = true;      // Set isListening flag

                RE::DebugNotification("Input Triggered - Start listening!");

                /// *** do stuff to activate listening of C# app

            } else {
                isListening = false;  // Reset isListening flag
                RE::DebugNotification("Input released - Stop Listening!");

                /// *** do stuff to deactivate listening of C# app
            }
            while (button->IsPressed()) Sleep(250);  // Pause while input trigger is still being pressed
        })
            .detach();
    }
}

#pragma endregion Device input triggers for speech recognition listening

//Initializes Plugin, Speech Recognition, Websocket, and data tracker
SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    try {
        SKSE::Init(skse);  // Initialize SKSE plugin
        ///MessageBoxA(NULL, "Press OK when you're ready!", "Skyrim", MB_OK | MB_ICONQUESTION);  // MessageBox to halt execution so a debugger can be attached
        SetupLog();  // Set up the debug logger

        SKSE::GetMessagingInterface()->RegisterListener(OnMessage); // Listen for game messages (usually "Is Game Loaded" comes first) and execute OnMessage method in response
        logger::info("Voxima finished loading");  // Temporary debug line so I can see the program started
        return true;
    
    } catch (exception ex) {
        logger::info("ERROR initializing Voxima SKSE plugin: {}", ex.what());
        return false;
    }
} // End SKSEPluginLoad


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
                RE::DebugNotification("Start listening!");

                /// *** do stuff to activate listening of C# app

                while (gamepad->IsPressed(rShoulder)) Sleep(250);   // Pause while gamepad trigger is still being pressed
                RE::DebugNotification("Stop listening!");
                allowTriggerProcessing = true;                      // Flag that gamepad trigger processing is allowed

                /// *** do stuff to deactivate listening of C# app

            }).detach();
        } else {
            thread([gamepad, rShoulder]() {                         // Create new thread for execution
                if (isListening == false) {                         // Check if recognition app is NOT listening
                    isListening = true;                             // Set isListening flag
                    RE::DebugNotification("Start listening!");

                    /// *** do stuff to activate listening of C# app

                } else {
                    isListening = false;                            // Reset isListening flag
                    RE::DebugNotification("Stop listening!");

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

#pragma endregion