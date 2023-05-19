#include "../functions/logger.hpp"  // SKSE log functions

RE::TESForm* currentVoice;
RE::PlayerCharacter* player;

// Global Variables
#pragma region In-Game Global Variables
RE::TESGlobal* VOX_Enabled;              // Enables or Disables Voice Recognition
RE::TESGlobal* VOX_UpdateInterval;       // Decides the update interval for loop-based CheckUpdate()
RE::TESGlobal* VOX_CheckForUpdate;       // Forces the system to check for an update. Also checks for a change in microphone
RE::TESGlobal* VOX_PushToSpeakType;      // Determines the type of push to speak. [0] = None. [1] = Hold. [2] = Toggle. [3] = Vocal
RE::TESGlobal* VOX_PushToSpeakKeyCode;   // The keycode (mouse, keyboard, gamepad, or VR) of the push-to-speak button
RE::TESGlobal* VOX_AutoCastPowers;       // Enables or Disables always auto-casting powers
RE::TESGlobal* VOX_AutoCastShouts;       // Enables or Disables always auto-casting shouts
RE::TESGlobal* VOX_ShowLog;              // Enables or Disables the notification log in the top left corner
RE::TESGlobal* VOX_ShoutKey;             // Determines the shout key that is press when a shout/power is cast
RE::TESGlobal* VOX_LongAutoCast;         // Enables or Disables casting spells that have a long casting time (like Flame Thrall)
RE::TESGlobal* VOX_Sensitivity;          // Determines how sensitive the voice recognition is
RE::TESGlobal* VOX_KnownShoutWordsOnly;  // Determines if you only get voice commands for shout words that are known, instead of all words after the first is learned
std::string openMenu;
#pragma endregion

#pragma region Enumerations and Structs
enum ActorSlot { Left, Right, Both, Voice, None };
enum MagicType { null, Spell, Power, Shout };
enum MenuType { Console, Favorites, Inventory, Journal, LevelUp, Magic, Map, Skills, SleepWait, Tween };
enum MenuAction { Open, Close };
enum MoveType { MoveForward, MoveLeft, MoveRight, MoveBackward, TurnLeft, TurnRight, MoveJump, MoveSprint, StopSprint, StopMoving }; //Horse Controls

//Pre-made messages that the App is prepared to recieve
struct WebSocketMessage
{
    static constexpr const char* EnableRecognition          = "enable recognition";
    static constexpr const char* DisableRecognition         = "disable recognition";

    static constexpr const char* CheckForMicChange          = "check for mic change";
    static constexpr const char* InitializeUpdate           = "initialize update";
    static constexpr const char* UpdateConfiguration        = "update configuration\n";

    static constexpr const char* UpdateSpells               = "update spells\n";
    static constexpr const char* UpdatePowers               = "update powers\n";
    static constexpr const char* UpdateShouts               = "update shouts\n";

    static constexpr const char* UpdateLocations            = "update locations\n";
    static constexpr const char* EnableLocationCommands     = "enable location commands";
    static constexpr const char* DisableLocationCommands    = "disable location commands";
};

// Value of a given actor slot (for equipping)
struct ActorSlotValue
{
    RE::BGSEquipSlot* leftHand()
    {
        const REL::Relocation<RE::BGSEquipSlot* (*)()> func{REL::VariantID(23150, 23607, 0x340EB0)};
        return func();
    }

    RE::BGSEquipSlot* rightHand()
    {
        const REL::Relocation<RE::BGSEquipSlot* (*)()> func{REL::VariantID(23151, 23608, 0x340EE0)};
        return func();
    }

    RE::BGSEquipSlot* voice()
    {
        const REL::Relocation<RE::BGSEquipSlot* (*)()> func{REL::VariantID(23153, 23610, 0x340F40)};
        return func();
    }
};

ActorSlotValue actorSlot;

#pragma endregion

#pragma region Function Definitions
bool IsPlayerWerewolf();
bool IsPlayerVampireLord();
int PlayerMorph();
int PlayerMount();
bool IsGodMode();
void CastVoice(RE::Actor* actor, RE::TESForm* item, int level);
std::vector<std::string> GetShoutList();
bool CastMagic(RE::Actor* actor, RE::TESForm* item, ActorSlot hand, int level);
static std::string* GetActorMagic(RE::Actor* player, MagicType type1, MagicType type2 = MagicType::null, MagicType type3 = MagicType::null);
void EquipToActor(RE::Actor* actor, RE::TESForm* item, ActorSlot hand, bool notification = true);
void UnEquipFromActor(RE::Actor* actor, ActorSlot hand);
static void SendKeyDown(int keycode);
static void SendKeyUp(int keycode);
void PressKey(int keycode, int duration = 0);
static bool IsKeyDown(int keyCode);
static void MoveHorse(MoveType moveType);
void ExecuteConsoleCommand(std::vector<std::string> command);
void SendJoystickInput();
void SendNotification(std::string message);
void MenuInteraction(MenuType type, MenuAction action);
std::string TranslateL33t(std::string string);
#pragma endregion

// Returns whether the actor is in God Mode
bool IsGodMode()
{
    REL::Relocation<bool*> singleton{REL::VariantID(517711, 404238, 0x2FFFDEA)};
    return *singleton;
}

// Brings the game window to the foreground
void SetWindowToFront()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);

    if (HWND windowHandle = FindWindow(NULL, (LPWSTR)(wchar_t*)std::wstring(buffer, buffer + strlen(buffer) + 1).c_str())) {
        ShowWindow(windowHandle, SW_RESTORE);
        SetForegroundWindow(windowHandle);
        SetFocus(windowHandle);
    }
}

// Returns the actor's current mount.  (0 = None)  (1 = Horse)  (2 = Dragon)
int PlayerMount()
{
    RE::ActorPtr mount;
    std::string mountName;

    if (player->GetMount(mount)) {
        mountName = mount->GetRace()->GetName();

        if (mountName == "Horse") {
            logger::info("Riding Horse");
            return 1;
        }

        if (mountName == "Dragon Race") {
            logger::info("Riding Dragon");
            return 2;
        }
    }

    logger::info("Riding Nothing");
    return 0;
}

// Translate L33t characters into corresponding english characters
std::string TranslateL33t(std::string string)
{
    /*
        1 = aa
        2 = ei
        3 = ii
        4 = ah
        5 = ?   (There are no instances of this L33t character in Vanilla or DLC shouts)
        6 = ur
        7 = ir
        8 = oo
        9 = ey
    */

    std::string finalString = string;

    std::string l33tTranslation[8][2] = {{"1", "aa"}, {"2", "ei"}, {"3", "ii"}, {"4", "ah"}, {"6", "ur"}, {"7", "ir"}, {"8", "oo"}, {"9", "ey"}};

    size_t pos = 0;

    for (std::string* item : l33tTranslation) {
        pos = 0;
        while ((pos = finalString.find(item[0], pos)) != std::string::npos) {
            finalString.replace(pos, item[0].length(), item[1]);
            pos += item[1].length();
        }
    }

    return finalString;
}

// Execute Console Commands
void ExecuteConsoleCommand(std::vector<std::string> command)
{
    const auto scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
    const auto script = scriptFactory ? scriptFactory->Create() : nullptr;
    if (script) {
        const auto selectedRef = RE::Console::GetSelectedRef();

        for (std::string item : command) {
            // Check for special commands
            if (item.starts_with("wait")) {
                if (item.ends_with('m')) {
                    std::this_thread::sleep_for(std::chrono::minutes(std::stoi(item.replace(0, 4, "").replace(item.length() - 1, item.length() - 1, ""))));
                }
                else if (item.ends_with('s')) {
                    std::this_thread::sleep_for(std::chrono::seconds(std::stoi(item.replace(0, 4, "").replace(item.length() - 1, item.length() - 1, ""))));
                }
                else if (item.ends_with("ms")) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(item.replace(0, 4, "").replace(item.length() - 2, item.length() - 1, ""))));
                }
            }
            else {
                // logger::info("Executing console command '{}'", item);
                script->SetCommand(item);
                script->CompileAndRun(selectedRef.get());
            }
        }  // End if/else
    }      // End for

    delete script;
}  // End ExecuteConsoleCommand

// Command horse to execute a given movement type
static void MoveHorse(MoveType moveType)
{
    // Overview of how the horse is moved for VR
    /*   When "W" is recieved, the horseDirection will be set to the closest 8-degree direction that the horse is facing
     *   All other commands will adjust the direction
     *   When the horse stops moving, the direction will be recet to stationary.
     *   If any user input is recieved, the directino is reset to stationary
     */

    RE::ActorPtr horse;
    bool move = false;

   (void)player->GetMount(horse);

    // VR-specific control
    if (REL::Module::IsVR()) {
        // Get Facing
        float angleZ = horse->GetAngleZ();

        // Adjust the angle to a 360 degree metric, from a 6.3 degree metric
        angleZ = (float)(angleZ * 360 / 6.3);

        // Adjust the angle to 8-degrees of freedom, from 360-degrees of freedom
        angleZ = roundf(angleZ / 45) * 45;

        // Adjust the horse as needed
        switch (moveType) {
            case MoveType::MoveForward:
                move = true;
                break;

            case MoveType::MoveLeft:
                move = true;
                // Rotate "HorseDirection" by 90 to the left
                angleZ -= 90;
                break;

            case MoveType::TurnLeft:
                move = true;
                // Rotate "HorseDirection" by 45 to the left
                angleZ -= 45;
                break;

            case MoveType::MoveRight:
                move = true;
                // Rotate "HorseDirection" by 90 to the right
                angleZ += 90;
                break;

            case MoveType::TurnRight:
                move = true;
                // Rotate "HorseDirection" by 45 to the right
                angleZ += 45;
                break;

            case MoveType::MoveBackward:
                move = true;
                // Rotate "HorseDirection" by 180
                angleZ += 180;

                if (angleZ > 360) angleZ -= 360;
                break;

            case MoveType::MoveJump:
                PressKey(57);  // Press "Space Bar"
                break;

            case MoveType::StopMoving:
                // Stop all movement
                SendKeyUp(17);  // Direction (N)
                SendKeyUp(32);  // Direction (E)
                SendKeyUp(31);  // Direction (S)
                SendKeyUp(30);  // Direction (W)
                SendKeyUp(30);  // Direction (W)
                SendKeyUp(42);  // Sprint    (Shift)
                break;

            case MoveType::MoveSprint:
                if (!horse->IsRunning()) PressKey(42);  // Press "Left Shift"
                break;

            case MoveType::StopSprint:
                if (horse->IsRunning()) PressKey(42);  // Press "Left Shift"
                break;
        }

        // Move the horse in desired direction
        if (move == true) {
            switch ((int)angleZ) {
                case 0:
                case 360:
                    SendKeyDown(17);  // Direction (N)
                    SendKeyUp(32);    // Direction (E)
                    SendKeyUp(31);    // Direction (S)
                    SendKeyUp(30);    // Direction (W)
                    break;

                case 45:
                    SendKeyDown(17);  // Direction (N)
                    SendKeyDown(32);  // Direction (E)
                    SendKeyUp(31);    // Direction (S)
                    SendKeyUp(30);    // Direction (W)
                    break;

                case 90:
                    SendKeyUp(17);    // Direction (N)
                    SendKeyDown(32);  // Direction (E)
                    SendKeyUp(31);    // Direction (S)
                    SendKeyUp(30);    // Direction (W)
                    break;

                case 135:
                    SendKeyUp(17);    // Direction (N)
                    SendKeyDown(32);  // Direction (E)
                    SendKeyDown(31);  // Direction (S)
                    SendKeyUp(30);    // Direction (W)

                    break;

                case 180:
                    SendKeyUp(17);    // Direction (N)
                    SendKeyUp(32);    // Direction (E)
                    SendKeyDown(31);  // Direction (S)
                    SendKeyUp(30);    // Direction (W)
                    break;

                case 225:
                    SendKeyUp(17);    // Direction (N)
                    SendKeyUp(32);    // Direction (E)
                    SendKeyDown(31);  // Direction (S)
                    SendKeyDown(30);  // Direction (W)
                    break;

                case 270:
                    SendKeyUp(17);    // Direction (N)
                    SendKeyUp(32);    // Direction (E)
                    SendKeyUp(31);    // Direction (S)
                    SendKeyDown(30);  // Direction (W)
                    break;

                case 315:
                    SendKeyDown(17);  // Direction (N)
                    SendKeyUp(32);    // Direction (E)
                    SendKeyUp(31);    // Direction (S)
                    SendKeyDown(30);  // Direction (W)
                    break;
            }
        }
    }
    else {
        // Player SE or AE
        switch (moveType) {
            case MoveType::MoveForward:;
                SendKeyDown(17);  // Direction (N)
                SendKeyUp(32);    // Direction (E)
                SendKeyUp(31);    // Direction (S)
                SendKeyUp(30);    // Direction (W)
                break;

            case MoveType::TurnRight:
                SendKeyDown(17);  // Direction (N)
                SendKeyDown(32);  // Direction (E)
                SendKeyUp(31);    // Direction (S)
                SendKeyUp(30);    // Direction (W)
                break;

            case MoveType::MoveRight:
                SendKeyUp(17);    // Direction (N)
                SendKeyDown(32);  // Direction (E)
                SendKeyUp(31);    // Direction (S)
                SendKeyUp(30);    // Direction (W)
                break;

            case MoveType::MoveBackward:
                SendKeyUp(17);    // Direction (N)
                SendKeyUp(32);    // Direction (E)
                SendKeyDown(31);  // Direction (S)
                SendKeyUp(30);    // Direction (W)
                break;

            case MoveType::MoveLeft:
                SendKeyUp(17);    // Direction (N)
                SendKeyUp(32);    // Direction (E)
                SendKeyUp(31);    // Direction (S)
                SendKeyDown(30);  // Direction (W)
                break;

            case MoveType::TurnLeft:
                SendKeyDown(17);  // Direction (N)
                SendKeyUp(32);    // Direction (E)
                SendKeyUp(31);    // Direction (S)
                SendKeyDown(30);  // Direction (W)
                break;

            case MoveType::MoveJump:
                PressKey(57);  // Press "Space Bar"
                break;

            case MoveType::StopMoving:
                SendKeyUp(17);  // Direction (N)
                SendKeyUp(32);  // Direction (E)
                SendKeyUp(31);  // Direction (S)
                SendKeyUp(30);  // Direction (W)
                break;

            case MoveType::MoveSprint:
                if (!horse->IsRunning()) PressKey(42);  // Press "Left Shift"
                break;

            case MoveType::StopSprint:
                if (horse->IsRunning()) PressKey(42);  // Press "Left Shift"
                break;
        }
    }

    /*
     * Potential Alternatives for the future:
     *   It's possible that moving the mouse will rotate the horse, causing "W" to move the horse in the new direction
     *   I may be able to use the VR controller's joystick to move just the horse. Maybe the code that move sthe horse in the actor's direction isn't directly tied to the movement
     * of the joystick Maybe a gamepad joystick can be simulated. It's possible that hte game will recognize the VR controllers as joysticks RE the code for WASD to see what
     * exactly is happening when you press those keys. Perhaps I can manually do it, and move in any direction I want
     */
}

// Sends a notification to the top left in Skyrim, if the actor has logs enabled
void SendNotification(std::string message)
{
    if (VOX_ShowLog->value == 1) {
        RE::DebugNotification(message.c_str());
    }
}

#pragma region Player Morph Checks
// Returns whether the actor is in Werewolf form
bool IsPlayerWerewolf()
{
    int currentWerewolfState = RE::TESQuest::LookupByEditorID<RE::TESQuest>("PlayerWerewolfQuest")->GetCurrentStageID();
    if (currentWerewolfState > 0 && currentWerewolfState < 100)
        return true;
    else
        return false;
}

// Returns whether the actor is in Vampire Lord form
bool IsPlayerVampireLord()
{
    int currentVampireLordState = RE::TESQuest::LookupByEditorID<RE::TESQuest>("DLC1PlayerVampireQuest")->GetCurrentStageID();
    if (currentVampireLordState > 0 && currentVampireLordState < 100)
        return true;
    else
        return false;
}

// Returns the actor's current morph.  (0 = None)  (1 = Werewolf)  (2 = Vampire Lord)
int PlayerMorph()
{
    if (IsPlayerWerewolf())
        return 1;
    else if (IsPlayerVampireLord()) 
        return 2;
    else
        return 0;
}

#pragma endregion Determines the players current morph (None, Werewolf, or Vampire Lord)

#pragma region Equip / Unequip / Casting
// Equip an item to an actor
void EquipToActor(RE::Actor* actor, RE::TESForm* item, ActorSlot hand, bool notification)
{
    try {
        /*
         *
         * When inventory functionality is added, this function will be used to equip items/armor, too
         * Probably also for using potions/scrolls
         */

        // Get Actor Process to check for currently equipped items
        //RE::AIProcess* actorProcess = actor->GetActorRuntimeData().currentProcess;
        std::vector<std::string> commands;
        std::string currentCommand;

        // These caused CTD when used, but these are how you would check if something i already in your hand
        /// RE::TESForm* left_hand = actorProcess->GetEquippedLeftHand();
        /// RE::TESForm* right_hand = actorProcess->GetEquippedRightHand();
        /// RE::TESForm* voice = actor->GetActorRuntimeData().selectedPower;

        // If the item does not exist, i.e. a nullptr
        if (!item) {
            logger::error("ERROR::Item cannot be equipped. It is NULL");
            return;
        }

        if (item->GetKnown() || actor->HasSpell(item->As<RE::SpellItem>())) {
            //  Spell/Power
            if (item->As<RE::SpellItem>()) {
                RE::SpellItem* spell = item->As<RE::SpellItem>();
                std::string spellName = spell->GetFullName();

                switch (hand) {
                    case ActorSlot::Left:
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.leftHand());
                        if (notification) SendNotification("Equipping: " + spellName + " Left");
                        break;

                    case ActorSlot::Right:
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.rightHand());
                        if (notification) SendNotification("Equipping: " + spellName + " Right");
                        break;

                    case ActorSlot::Both:
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.leftHand());
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.rightHand());
                        if (notification) SendNotification("Equipping: " + spellName + " Both");
                        break;

                    case ActorSlot::Voice:
                        RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.voice());
                        if (notification) SendNotification("Equipping Power: " + spellName);
                        break;
                }

                // Shout
            }
            else if (item->As<RE::TESShout>()) {
                RE::TESShout* shout = item->As<RE::TESShout>();
                std::string shoutName = shout->GetFullName();

                RE::ActorEquipManager::GetSingleton()->EquipShout(actor, shout);
                if (notification) SendNotification("Equipping Shout: " + shoutName);
            }
        }
        else {
            logger::info("Item not equipped. Player does not know it");
        }  // End check if actor knows item

        // logger::info("Item Equipped");
    }
    catch (const std::exception& ex) {
        logger::error("ERROR: {}", ex.what());
    }
}

// Unequip item from actor slot (only works for the actor at the moment)
void UnEquipFromActor(RE::Actor* actor, ActorSlot hand)
{
    // Unequipping functions
    // Unequips a spell from the actor's hand or power from their voice
    using spell = void(RE::BSScript::IVirtualMachine * a_vm, RE::VMStackID a_stack_id, RE::Actor * actor, RE::SpellItem * a_spell, uint32_t a_slot);
    const REL::Relocation<spell> un_equip_spell{REL::VariantID(227784, 54669, 0x984D00)};

    // Unequips a shout from the actor
    using shout = void(RE::BSScript::IVirtualMachine * a_vm, RE::VMStackID a_stack_id, RE::Actor * actor, RE::TESShout * a_shout);
    const REL::Relocation<shout> un_equip_shout{REL::VariantID(53863, 54664, 0x984C60)};

    // Get Actor Process to check for currently equipped items
    auto actorProcess = actor->GetActorRuntimeData().currentProcess;

    bool unEquipSound = true;  // This only applies to items, not spells

    std::vector<std::string> commands;
    std::string currentCommand;

    RE::TESForm* left_hand = actorProcess->GetEquippedLeftHand();
    RE::TESForm* right_hand = actorProcess->GetEquippedRightHand();
    RE::TESForm* voice = actor->GetActorRuntimeData().selectedPower;

    switch (hand) {
        case ActorSlot::Left:
        case ActorSlot::Right:
        case ActorSlot::Both:
            if (left_hand)
                if (left_hand->Is(RE::FormType::Spell))
                    un_equip_spell(nullptr, 0, actor, left_hand->As<RE::SpellItem>(), 0);
                else
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(actor, left_hand->As<RE::TESBoundObject>(), nullptr, 1U, actorSlot.leftHand(), false, false, unEquipSound);

            if (right_hand)
                if (right_hand->Is(RE::FormType::Spell))
                    un_equip_spell(nullptr, 0, actor, right_hand->As<RE::SpellItem>(), 1);
                else
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(actor, right_hand->As<RE::TESBoundObject>(), nullptr, 1U, actorSlot.rightHand(), false, false,
                                                                         unEquipSound);
            break;

        case ActorSlot::Voice:
            if (voice) {
                // shout
                if (voice->Is(RE::FormType::Shout)) {
                    un_equip_shout(nullptr, 0, actor, voice->As<RE::TESShout>());

                    // Power
                }
                else if (voice->Is(RE::FormType::Spell)) {
                    un_equip_spell(nullptr, 0, actor, voice->As<RE::SpellItem>(), 2);
                }
            }
            break;
    }
}

// Cast Magic from actor slot. Returns whether the cast was successful
bool CastMagic(RE::Actor* actor, RE::TESForm* item, ActorSlot hand, int modifier = 0)
{
    //Modifier meaning
    /*
     * modifier can mean 2 things :
     * (1) The shout level
     * (2) If the dual-Casting perk should be taken into account (0 == false. 1 == true)
     *
     */

    if (item->GetKnown() || actor->HasSpell(item->As<RE::SpellItem>()))
    {
        // Shout
        if (item->As<RE::TESShout>())
        {
            logger::info("Casting Voice");
            std::string shoutWordName = item->As<RE::TESShout>()->variations[modifier].spell->fullName.c_str(); // Capture the target shout words
            currentVoice = actor->GetActorRuntimeData().selectedPower;  // Capture currently equipped shout/power
            EquipToActor(actor, item, ActorSlot::Voice, false);         // Equip target shout
            SendNotification("Casting Shout: " + shoutWordName);
            std::thread castShout(CastVoice, actor, item, modifier);    // Create new thread to cast shout via button spoofing
            castShout.detach();                                         // Run new thread

            /* // This is the "legacy method" involving triggering a virtual keypress. Dovazul and thunderclap expected to work.
            EquipToActor(actor, item, ActorSlot::Voice, false);
            SendNotification("Casting Shout: " + shoutWordName);
            std::thread castShout(CastVoice, actor, item, modifier);
            castShout.detach(); */

            /* // Directly casts the shout, but it doesn't trigger the dovazul voice or thunderclap sound
            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(item->As<RE::TESShout>()->variations[modifier].spell, false, nullptr, 1.0f, false, 0.0f, actor); */

            /* // Spoof button input to cast shout (see UserEvents.h for more types of events to spoof)
            // https://discord.com/channels/535508975626747927/535530099475480596/1093045153742200895, https://www.creationkit.com/index.php?title=Shout
            EquipToActor(actor, item, ActorSlot::Voice, false);
            if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
                float holdTime = 0;
                switch (modifier) {
                    case 0:
                        holdTime = 0.075f;
                        // No Delay
                        break;
                    case 1:
                        holdTime = 0.25f;
                        break;

                    case 2:
                        holdTime = 0.75f;
                        break;
                }
                SendNotification(std::to_string(holdTime));
                auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 1.0f, holdTime);
                bsInputEventQueue->PushOntoInputQueue(kEvent);
            } */

            /* // Spoof button input to cast shout (see UserEvents.h for more types of events to spoof)
            // https://discord.com/channels/535508975626747927/535530099475480596/1093045153742200895, https://www.creationkit.com/index.php?title=Shout
            EquipToActor(actor, item, ActorSlot::Voice, false);
            if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
                float holdTime = 0;
                RE::ButtonEvent* endEvent = nullptr;
                static auto startEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 1.0f, 0.0f);
                static auto eventMod0 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 0.0f, 0.010f);
                static auto eventMod1 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 0.0f, 0.075f);
                static auto eventMod2 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 0.0f, 2.0f);
                
                switch (modifier) {
                    case 0:
                        endEvent = eventMod0;
                        break;
                    case 1:
                        endEvent = eventMod1;
                        break;
                    case 2:
                        endEvent = eventMod2;
                        break;
                }
                bsInputEventQueue->PushOntoInputQueue(startEvent);
                bsInputEventQueue->PushOntoInputQueue(endEvent);
            } */

            /* // This seems to work
            ///EquipToActor(actor, item, ActorSlot::Voice, false);
            std::thread([actor, item, modifier]() {
                // Spoof button input to cast shout (see UserEvents.h for more types of events to spoof)
                // https://discord.com/channels/535508975626747927/535530099475480596/1093045153742200895
                if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
                    int holdTime = 0;
                    switch (modifier) {
                        case 0:
                            holdTime = 10;
                            break;
                        case 1:
                            holdTime = 75;
                            break;
                        case 2:
                            holdTime = 300;
                            break;
                    }
                    // Here we're attempting to emulate the button "press," pause (indicating "hold"), and "release" sequence
                    auto kEvent1 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 1.0f, 0.0f);
                    auto kEvent2 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 0.0f, 0.0f);
                    bsInputEventQueue->PushOntoInputQueue(kEvent1);
                    Sleep(holdTime);
                    bsInputEventQueue->PushOntoInputQueue(kEvent2);
                }
            }).detach(); */
        }
        // Power
        else if (item->As<RE::SpellItem>() && hand == ActorSlot::Voice) {
            logger::info("Casting Power");
            std::string spellName = item->As<RE::MagicItem>()->fullName.c_str(); // Capture the target power name
            currentVoice = actor->GetActorRuntimeData().selectedPower;  // Capture currently equipped shout/power
            EquipToActor(actor, item, ActorSlot::Voice, false);         // Equip target power
            SendNotification("Casting Power: " + spellName);
            std::thread castShout(CastVoice, actor, item, modifier);    // Create new thread to cast power via button spoofing
            castShout.detach();                                         // Run new thread

            /* // This is the "legacy method" involving triggering a virtual keypress.
            EquipToActor(actor, item, ActorSlot::Voice);
            SendNotification("Casting Power: " + spellName);
            std::thread castShout(CastVoice, actor, item, modifier);
            castShout.detach(); */

            /* // Directly cast the power. However this bypasses in-game cooldown for powers
            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(item->As<RE::MagicItem>(), false, nullptr, 1.0f, false, 0.0f, actor); */

            /* // Spoof pressing the button that triggers Powers (doesn't work as configured currently)
            EquipToActor(actor, item, ActorSlot::Voice);
            if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
                auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 1.0f, 0.0f);
                bsInputEventQueue->PushOntoInputQueue(kEvent);
            } */

            /* EquipToActor(actor, item, ActorSlot::Voice, false);
            std::thread([actor, item, modifier]() {
                // Spoof button input to cast shout (see UserEvents.h for more types of events to spoof)
                // https://discord.com/channels/535508975626747927/535530099475480596/1093045153742200895
                if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
                    int holdTime = 10;
                    // Here we're attempting to emulate the button "press," pause (indicating "hold"), and "release" sequence
                    auto kEvent1 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 1.0f, 0.0f);
                    auto kEvent2 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 0.0f, 0.0f);
                    bsInputEventQueue->PushOntoInputQueue(kEvent1);
                    Sleep(holdTime);
                    bsInputEventQueue->PushOntoInputQueue(kEvent2);
                }
            }).detach(); */
        }
        // Spell
        else if (item->As<RE::SpellItem>()) {
            RE::MagicItem* spell = item->As<RE::MagicItem>();
            float singleMagickaCost = spell->CalculateMagickaCost(actor);
            float duelMagickaCost = singleMagickaCost * RE::GameSettingCollection::GetSingleton()->GetSetting("fMagicDualCastingCostMult")->GetFloat();
            
            float actorMagicka = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka);
            bool accountForDualPerk = false;

            bool canCastSingle = actorMagicka >= singleMagickaCost;
            bool canCastDual = actorMagicka >= duelMagickaCost;
            std::string spellName = spell->fullName.c_str();

            // Duel Casting Perks
            RE::BGSPerk* dualAlterationPerk = RE::BGSPerk::LookupByID<RE::BGSPerk>(0x000153CD);
            RE::BGSPerk* dualConjurationPerk = RE::BGSPerk::LookupByID<RE::BGSPerk>(0x000153CE);
            RE::BGSPerk* dualDestructionPerk = RE::BGSPerk::LookupByID<RE::BGSPerk>(0x000153CF);
            RE::BGSPerk* dualIllusionPerk = RE::BGSPerk::LookupByID<RE::BGSPerk>(0x000153D0);
            RE::BGSPerk* dualRestorationPerk = RE::BGSPerk::LookupByID<RE::BGSPerk>(0x000153D1);

            if (modifier == 1 && (canCastDual || IsGodMode()))
            {
                switch (spell->GetAssociatedSkill())
                {
                    case RE::ActorValue::kAlteration:
                        accountForDualPerk = actor->HasPerk(dualAlterationPerk);
                        //SendNotification("Alteration");
                        break;

                    case RE::ActorValue::kConjuration:
                        accountForDualPerk = actor->HasPerk(dualConjurationPerk);
                        //SendNotification("Conjuration");
                        break;

                    case RE::ActorValue::kDestruction:
                        accountForDualPerk = actor->HasPerk(dualDestructionPerk);
                        //SendNotification("Destruction");
                        break;

                    case RE::ActorValue::kIllusion:
                        accountForDualPerk = actor->HasPerk(dualIllusionPerk);
                        //SendNotification("Illusion");
                        break;

                    case RE::ActorValue::kRestoration:
                        accountForDualPerk = actor->HasPerk(dualRestorationPerk);
                        //SendNotification("Restoration");
                        break;

                        default:
                        SendNotification("Unknown Skill Tree");
                        break;
                }
            }

            if (canCastSingle || canCastDual || IsGodMode()) {

                // Damage Magicka
                if (accountForDualPerk)
                        actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -duelMagickaCost);
                else
                        actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -singleMagickaCost);
                
                switch (hand) {
                    case ActorSlot::Left:
                        logger::info("Casting Left hand");
                        SendNotification("Casting: " + spellName + " Left");


                        // Enable Dual Casting Perk, if applicable
                        if (accountForDualPerk) actor->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)->SetDualCasting(true);

                        // Cast Spell
                        actor->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, actor);

                        // Disabled Dual Casting Perk
                        if (accountForDualPerk) actor->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)->SetDualCasting(false);

                        return true;

                        break;

                    case ActorSlot::Right:
                        logger::info("Casting Right hand");
                        SendNotification("Casting: " + spellName + " Right");

                        // Enable Dual Casting Perk, if applicable
                        if (accountForDualPerk) actor->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand)->SetDualCasting(true);

                        // Cast Spell
                        actor->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand)->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, actor);

                        //Disabled Dual Casting Perk
                        if (accountForDualPerk) actor->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand)->SetDualCasting(false);

                        return true;

                        break;

                    case ActorSlot::Both:
                        if (canCastDual || IsGodMode()) {
                            logger::info("Casting Both hands");
                            SendNotification("Casting: " + spellName + " Dual");

                            // Cast From Left Hand
                            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, actor);

                            // Cast From Right Hand
                            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand)->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, actor);

                            // Remove Magicka again
                            actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -singleMagickaCost);

                            return true;
                        }
                        break;
                }//End Switch
            }//End Can Cast
        }//End is valid item
    }//End is known item
    else {
        logger::info("Item not cast. Player does not know it");
    }  // Determine if the actor knows the item

    return false;
}

// Asynconously casts a shout/power from the actor
void CastVoice(RE::Actor* actor, RE::TESForm* item, int level)
{
    // Spoof button input to cast shout/power
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
        int holdTime = 10;
        if (item->As<RE::TESShout>()) {
            switch (level) {
                case 0:
                    // No change to holdTime
                    break;
                case 1:
                    holdTime = 75;
                    break;
                case 2:
                    holdTime = 2000;
                    break;
            }
        }
        // Spoof the button "press," pause (indicating "hold"), and "release" event sequence
        auto kEvent1 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 1.0f, 0.0f);
        auto kEvent2 = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "shout", 0, 0.0f, 0.0f);
        bsInputEventQueue->PushOntoInputQueue(kEvent1);
        Sleep(holdTime);
        bsInputEventQueue->PushOntoInputQueue(kEvent2);
        Sleep(500);  // Brief delay to allow the button spoofing to finish
    }
    
    // Re-equip last voice item. Unequip if there was no previous voice item
    if (!currentVoice)
        UnEquipFromActor(actor, ActorSlot::Voice);
    else
        EquipToActor(actor, currentVoice, ActorSlot::Voice, false);
}

/* // Asynconously casts a shout/power from the actor
void CastVoice(RE::Actor* actor, RE::TESForm* item, int level)
{
    // Get the key the actor designated as the key to press to activate shouts. Not automatic because VR says it's a controller button, which I can't press
    VOX_ShoutKey = RE::TESGlobal::LookupByEditorID<RE::TESGlobal>("VOX_ShoutKey");

    // Manually inserted this becaue the timing needs to be perfect

    // Move game to foreground and focus it
    SetWindowToFront();

    INPUT input = {};
    /// ZeroMemory(&input, sizeof(input));
    input.type = INPUT_KEYBOARD;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.wScan = (WORD)VOX_ShoutKey->value;

    SendInput(1, &input, sizeof(INPUT));

    // Shout
    if (item->As<RE::TESShout>()) {
        // float shouttime1 = RE::GameSettingCollection::GetSingleton()->GetSetting("fShoutTime1")->GetFloat();  //Time required to hold down shout key for shout level 2
        float shouttime2 = RE::GameSettingCollection::GetSingleton()->GetSetting("fShoutTime2")->GetFloat();  // Time required to hold down shout key for shout level 3

        switch (level) {
            case 0:
                // No Delay
                break;
            case 1:
                std::this_thread::sleep_for(std::chrono::milliseconds((int)(shouttime2 * 1000)));
                break;

            case 2:
                std::this_thread::sleep_for(std::chrono::milliseconds((int)(shouttime2 * 1000) + 100));
                break;
        }
    }

    // Manually inserted this becaue the timing needs to be perfect
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;

    SendInput(1, &input, sizeof(INPUT));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // Delay to allow the key function to finish

    // Re-equip last voice item. Unequip if there was no previous voice item
    if (!currentVoice)
        UnEquipFromActor(actor, ActorSlot::Voice);
    else
        EquipToActor(actor, currentVoice, ActorSlot::Voice);
} */

#pragma endregion Equipping, Unequipping, and Casting and items and magic

#pragma region Get Actor Items/Magic
// Retrieves known Shout and Word of Power data
std::vector<std::string> GetShoutList()
{
    std::vector<std::string> shoutList;
    auto playerSpells = player->GetActorBase()->GetSpellList();  // Obtain actor spell data
    RE::TESShout** playerShouts = playerSpells->shouts;          // Obtain all of actor's known shouts
    int numberOfShouts = (int)playerSpells->numShouts;               // Obtain number of actor's known shouts
    /// logger::debug("Number of Known Shouts = {}", numberOfShouts);
    try {
        for (int i = 0; i < numberOfShouts; i++) {            // Loop through each of the actor's known shouts
            RE::TESShout* shout = playerShouts[i];            // Capture the current shout at index i
            const char* shoutName = shout->fullName.c_str();  // Capture the name of the shout
            shoutList.push_back(shoutName);                   // Add the shoutName to the shoutList (thereby growing the list size)
            shoutList[i] += "\t" + std::format("{:X}", shout->GetLocalFormID()) + '\t' + "shout" + '\t' + shout->GetFile(0)->GetFilename().data();  // Add shout information
            /// logger::debug("Shout {} Name = {}", i + 1, shoutList[i]);
            for (int j = 0; j <= 2; j++) {                                    // Loop through all three shout words of power
                RE::TESWordOfPower* wordOfPower = shout->variations[j].word;  // Capture shout's word of power at j index
                if (wordOfPower &&
                    (VOX_KnownShoutWordsOnly->value == 0 ||
                     wordOfPower->formFlags & 0x10000)) {  // Check if current word of power is "shoutable" by actor (both known AND unlocked) and if actor wants to shout only known words
                    const char* wopName = wordOfPower->fullName.c_str();            // Capture name of known word of power (often contains L33T text)
                    std::string wopTranslation = wordOfPower->translation.c_str();  // Capture translation of known word of power
                    /// logger::debug("Shout \"{}\" Word {} = {} ({})", shoutName, j + 1, wopName, wopTranslation);
                    // shoutList[i] += "__" + wopTranslation;  // Append known word of power translation to current shout in shoutList
                    shoutList[i] += "\t" + TranslateL33t(wopName);  // Append translated words of power
                }
                else
                    break;  // Break out of parent "for" loop
            }
        }
        /// for (auto& shoutData : shoutList)                // Loop through all contents of shoutList
        //    logger::debug("ShoutList = {}", shoutData);  // Output contents of shoutData
    }
    catch (const std::exception& ex) {
        logger::error("ERROR during GetShoutList: {}", ex.what());
    }
    return shoutList;
}

// Get a list of Magic
static std::string* GetActorMagic(RE::Actor* actor, MagicType type1, MagicType type2, MagicType type3)
{
    int numTypes = 0;
    bool getSpells = false;
    bool getPowers = false;
    bool getShouts = false;

    std::string updateSpells = "";
    std::string updatePowers = "";
    std::string updateShouts = "";

    switch (type1) {
        case MagicType::Spell:
            getSpells = true;
            numTypes++;
            break;

        case MagicType::Power:
            getPowers = true;
            numTypes++;
            break;

        case MagicType::Shout:
            getShouts = true;
            numTypes++;
            break;
    }

    switch (type2) {
        case MagicType::Spell:
            getSpells = true;
            numTypes++;
            break;

        case MagicType::Power:
            getPowers = true;
            numTypes++;
            break;

        case MagicType::Shout:
            getShouts = true;
            numTypes++;
            break;
    }

    switch (type3) {
        case MagicType::Spell:
            getSpells = true;
            numTypes++;
            break;

        case MagicType::Power:
            getPowers = true;
            numTypes++;
            break;

        case MagicType::Shout:
            getShouts = true;
            numTypes++;
            break;
    }

    std::string* list = new std::string[numTypes];

    // If Spells or Powers
    if (getSpells || getPowers) {
        int numSpells[2] = {};

        // Get spells from Player Base
        RE::SpellItem** baseSpells = actor->GetActorBase()->GetSpellList()->spells;  // List of the actor's spells.
        numSpells[0] = actor->GetActorBase()->GetSpellList()->numSpells;             // The number of spells the actor has.

        // Get Spells from PlayerCharacter (all but Base)
        RE::SpellItem** raceSpells = actor->GetRace()->actorEffects->spells;  // List of the actor's spells.
        numSpells[1] = actor->GetRace()->actorEffects->numSpells;             // The number of spells the actor has.

        // Base Spells/Powers
        for (int i = 0; i < numSpells[0]; i++) {
            RE::SpellItem* spell = baseSpells[i];

            switch (spell->GetSpellType()) {
                case RE::MagicSystem::SpellType::kSpell:
                    if (getSpells) {
                        updateSpells += (std::string)spell->GetName() + '\t' + std::format("{:X}", spell->GetLocalFormID()) + '\t' + "spell" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;

                case RE::MagicSystem::SpellType::kPower:
                case RE::MagicSystem::SpellType::kLesserPower:
                case RE::MagicSystem::SpellType::kVoicePower:
                    if (getPowers) {
                        updatePowers += (std::string)spell->GetName() + '\t' + std::format("{:X}", spell->GetLocalFormID()) + '\t' + "power" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;
            }  // End Switch
        }      // End Base Spells/Powers

        // Racial Spells/Powers
        for (int i = 0; i < numSpells[1]; i++) {
            RE::SpellItem* spell = raceSpells[i];

            switch (spell->GetSpellType()) {
                case RE::MagicSystem::SpellType::kSpell:
                    if (getSpells) {
                        updateSpells += (std::string)spell->GetName() + '\t' + std::format("{:X}", spell->GetLocalFormID()) + '\t' + "spell" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;

                case RE::MagicSystem::SpellType::kPower:
                case RE::MagicSystem::SpellType::kLesserPower:
                case RE::MagicSystem::SpellType::kVoicePower:
                    if (getPowers) {
                        updatePowers += (std::string)spell->GetName() + '\t' + std::format("{:X}", spell->GetLocalFormID()) + '\t' + "power" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;
            }  // End Switch
        }      // End Racial Spells/Powers

        // Obtained Spells/Powers
        for (auto& spell : actor->GetActorRuntimeData().addedSpells) {
            switch (spell->GetSpellType()) {
                case RE::MagicSystem::SpellType::kSpell:
                    if (getSpells) {
                        updateSpells += (std::string)spell->GetName() + '\t' + std::format("{:X}", spell->GetLocalFormID()) + '\t' + "spell" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;

                case RE::MagicSystem::SpellType::kPower:
                case RE::MagicSystem::SpellType::kLesserPower:
                case RE::MagicSystem::SpellType::kVoicePower:
                    if (getPowers) {
                        updatePowers += (std::string)spell->GetName() + '\t' + std::format("{:X}", spell->GetLocalFormID()) + '\t' + "power" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;
            }  // End Switch
        }      // End Obtained Spells/Powers
    }          // End if Spells or Powers

    // Owned Shouts
    if (getShouts) {
        for (std::string shout : GetShoutList()) {
            updateShouts += shout + "\n";
        }

    }  // End Owned Shouts

    switch (type1) {
        case MagicType::Spell:
            list[0] = updateSpells;
            break;
        case MagicType::Power:
            list[0] = updatePowers;
            break;
        case MagicType::Shout:
            list[0] = updateShouts;
            break;
    }

    switch (type2) {
        case MagicType::Spell:
            list[1] = updateSpells;
            break;
        case MagicType::Power:
            list[1] = updatePowers;
            break;
        case MagicType::Shout:
            list[1] = updateShouts;
            break;
    }

    switch (type3) {
        case MagicType::Spell:
            list[2] = updateSpells;
            break;
        case MagicType::Power:
            list[2] = updatePowers;
            break;
        case MagicType::Shout:
            list[2] = updateShouts;
            break;
    }

    return list;
}
#pragma endregion Gets the players current Spells, Powers, and Shouts

#pragma region Key Output Management
// Briefly Press Key
void PressKey(int keycode, int milliseconds)
{
    SendKeyDown(keycode);
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    SendKeyUp(keycode);
}

// Hold Key Down
static void SendKeyDown(int keycode)
{
    SetWindowToFront();

    INPUT input;
    ZeroMemory(&input, sizeof(input));
    input.type = INPUT_KEYBOARD;
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    input.ki.wScan = (WORD)keycode;
    SendInput(1, &input, sizeof(INPUT));
}

// Release a Key
static void SendKeyUp(int keycode)
{
    SetWindowToFront();

    INPUT input;
    ZeroMemory(&input, sizeof(input));
    input.type = INPUT_KEYBOARD;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    input.ki.wScan = (WORD)keycode;

    SendInput(1, &input, sizeof(INPUT));
}

// Returns whether the specified keyboard or mouse key is held down
static bool IsKeyDown(int keyCode)
{
    int mouseOffset = 256;
    int vrRightOffset = 410;
    int vrLeftOffset = 474;
    //int gamepadOffset = null;

    // Mouse and keyboard input check
    bool isKeyboardKeyDown = RE ::BSInputDeviceManager::GetSingleton()->GetKeyboard()->IsPressed((uint32_t)keyCode);
    bool isMouseKeyDown = RE::BSInputDeviceManager::GetSingleton()->GetMouse()->IsPressed((uint32_t)keyCode - mouseOffset);
    bool isVrRightDown = RE::BSInputDeviceManager::GetSingleton()->GetVRControllerRight()->IsPressed((uint32_t)keyCode - vrRightOffset);
    bool isVrLeftDown = RE::BSInputDeviceManager::GetSingleton()->GetVRControllerRight()->IsPressed((uint32_t)keyCode - vrLeftOffset);

    //// VR controller input check
    // bool isLeftVRControllerKeyDown = RE::BSInputDeviceManager::GetSingleton()->GetVRControllerLeft()->IsPressed(VOX->value - vrLeftOffset);
    // bool isRightVRControllerKeyDown = RE::BSInputDeviceManager::GetSingleton()->GetVRControllerRight()->IsPressed(VOX_PushToSpeak->value - vrRightOffset);

    //// Gamepad input check
    // bool isGamepadKeyDown = RE::BSInputDeviceManager::GetSingleton()->GetGamepad()->IsPressed(VOX_PushToSpeak->value - gamepadOffset);

    return (isKeyboardKeyDown || isMouseKeyDown || isVrRightDown || isVrLeftDown);
}
#pragma endregion Control whether a key is down for Keyboard, Mouse, Gamepad, or VR 

#pragma region Menu Controls
// Open Journal menu (courtesy of shad0wshayd3)
void OpenJournal()
{
    // Spoof button input to open journal
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
        auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "journal", 0, 1.0f, 0.0f);
        bsInputEventQueue->PushOntoInputQueue(kEvent);
    }

    /* // Also works
    REL::Relocation<void (*)(bool)> func{REL::VariantID(52428, 53327, 0x92F0F0)};
    return func(true); */
}

// Open or close menu of interest
void MenuInteraction(MenuType type, MenuAction action)
{
    RE::BSFixedString menuName;
    RE::UI_MESSAGE_TYPE menuAction = RE::UI_MESSAGE_TYPE::kHide;
    if (action == MenuAction::Open)
        menuAction = RE::UI_MESSAGE_TYPE::kShow;
    /*else if (action == MenuAction::Close)
        menuAction = RE::UI_MESSAGE_TYPE::kHide;*/
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

        case MenuType::Magic:
            menuName = RE::MagicMenu::MENU_NAME;
            break;

        case MenuType::Map:
            menuName = RE::MapMenu::MENU_NAME;
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

        case MenuType::LevelUp:
            auto playerSkillData = player->GetInfoRuntimeData().skills->data;
            auto playerXP = playerSkillData->xp;
            auto playerLevelThreshold = playerSkillData->levelThreshold;
            if (playerXP >= playerLevelThreshold) {
                menuName = RE::StatsMenu::MENU_NAME;  // This will open the Skills menu, which will trigger the level up UI
                /// menuName = RE::LevelUpMenu::MENU_NAME; // This will directly call the level up UI
            }
            else {
                SendNotification("Not enough experience to level up");
                return;
            }
            break;
    }
    if (menuName == NULL) {  // Triggering menu is not of interest
        logger::error("Error processing menu action - unexpected enum encountered");
        return;
    }
    else if (action == MenuAction::Open && openMenu == std::string(menuName.c_str())) {  // Check if action is to open a menu AND the target menu is already open
        SendNotification("Menu already open");
        return;
    }
    else if (RE::UI::GetSingleton()->IsMenuOpen(RE::LevelUpMenu::MENU_NAME) == true) { // Check if LevelUp menu is currently open
        SendNotification("Menu already open");
        return;
    }
    std::thread([action, type, menuName, menuAction]() {
        if (action == MenuAction::Open && openMenu != "") { // Check if action is to open a menu AND the open menu does not match the requested menu
            auto* ui = RE::UI::GetSingleton();
            if (openMenu == RE::MapMenu::MENU_NAME && menuName == RE::JournalMenu::MENU_NAME) {  // Check if map is currently open and Journal was requested to open
                /// SendNotification("Open Journal");
                OpenJournal();  // Open the JournalMenu (Quests tab)
                return;
            }
            else if (ui->IsMenuOpen(RE::JournalMenu::MENU_NAME) == true && ui->IsMenuOpen(RE::MapMenu::MENU_NAME) == true) { // Check if journal and map are both open
                /// SendNotification("Auto Close Journal and Map");
                RE::UIMessageQueue::GetSingleton()->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);  // Send message to close the Journal
                Sleep(150); // Brief pause to ensure open menu is closed before proceeding
                RE::UIMessageQueue::GetSingleton()->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);  // Send message to close the Map
                Sleep(150); // Brief pause to ensure open menu is closed before proceeding
            }
            else {
                /// SendNotification("Auto Close " + openMenu);
                RE::UIMessageQueue::GetSingleton()->AddMessage(openMenu, RE::UI_MESSAGE_TYPE::kHide, nullptr);  // Send message to close the open menu
                Sleep(150);  // Brief pause to ensure open menu is closed before proceeding
            }
        }
        if (action == MenuAction::Open && menuName == RE::JournalMenu::MENU_NAME) {  // Check if Journal was requested to open
            /// SendNotification("Open Journal");
            OpenJournal(); // Open the JournalMenu (Quests tab)
        }
        else
            RE::UIMessageQueue::GetSingleton()->AddMessage(menuName, menuAction, nullptr); // Send message to open/close the target menu
    }).detach();
}

#pragma region Map Info and Manipulation

// Extend IUIMessageData and add refHandle member
class RefHandleUIData : public RE::IUIMessageData
{
public:
    uint32_t refHandle;  // 10
};

// Convert string to title case
std::string Title_Case(const std::string A)
{
    std::string B = "";

    int pos = 0;
    int pre_pos = 0;

    pos = A.find(' ', pre_pos);

    while (pos != std::string::npos) {
        std::string sub = "";

        sub = A.substr(pre_pos, (pos - pre_pos));

        if (pre_pos != pos) {
            sub = A.substr(pre_pos, (pos - pre_pos));
        }
        else {
            sub = A.substr(pre_pos, 1);
        }

        sub[0] = toupper(sub[0]);
        B += sub + A[pos];

        if (pos < (A.length() - 1)) {
            pre_pos = (pos + 1);
        }
        else {
            pre_pos = pos;
            break;
        }

        pos = A.find(' ', pre_pos);
    }

    std::string sub = A.substr(pre_pos, std::string::npos);
    sub[0] = toupper(sub[0]);
    B += sub;

    return B;
}

// Retrieve all map markers in the game (courtesy of Nightfallstorm)
RE::BSTArray<RE::ObjectRefHandle>* GetPlayerMapMarkers()
{
    std::uint32_t offset = 0;
    if (REL::Module::IsAE())
        offset = 0x500;
    else if (REL::Module::IsSE())
        offset = 0x4F8;
    else
        offset = 0xAE8;
    return reinterpret_cast<RE::BSTArray<RE::ObjectRefHandle>*>((uintptr_t)player + offset);
}

// Check if target location is known and return TESObjectREFR if applicable
RE::TESObjectREFR* IsLocationKnown(std::string targetLocation)
{
    auto* playerMapMarkers = GetPlayerMapMarkers();
    for (auto playerMapMarker : *playerMapMarkers) {
        const auto refr = playerMapMarker.get().get();
        if (refr->IsDisabled() == false) {  // Check if map marker is NOT disabled
            const auto marker = refr ? refr->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
            if (marker && marker->mapData) {
                if (marker->mapData->flags.any(RE::MapMarkerData::Flag::kVisible) == true) {  // Check if map marker is visible
                    std::string markerName = marker->mapData->locationName.GetFullName();
                    std::transform(targetLocation.begin(), targetLocation.end(), targetLocation.begin(), [](unsigned char c) { return std::tolower(c); });  // sets type.ToLower()
                    std::transform(markerName.begin(), markerName.end(), markerName.begin(), [](unsigned char c) { return std::tolower(c); });              // sets type.ToLower()

                    if (markerName == targetLocation) return refr;
                }
            }
        }
    }
    return NULL;
}

// Retrieve list of known locations (markers)
std::vector<std::string> GetKnownLocations()
{
    std::vector<std::string> locationList;
    auto* playerMapMarkers = GetPlayerMapMarkers();
    for (auto playerMapMarker : *playerMapMarkers) {
        const auto refr = playerMapMarker.get().get();
        if (refr->IsDisabled() == false) {  // Check if map marker is NOT disabled
            const auto marker = refr ? refr->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
            /// logger::debug("Game Location Name = {}", marker->mapData->locationName.GetFullName());
            if (marker && marker->mapData) {
                if (marker->mapData->flags.any(RE::MapMarkerData::Flag::kVisible) == true)  // Check if map marker is visible
                    locationList.push_back(marker->mapData->locationName.GetFullName());
            }
        }
    }
    /*for (auto location : locationList)
        logger::debug("Known Location Name = {}", location);*/
    return locationList;
}

// Create UI message for processing map auto navigation (courtesy of shad0wshayd3)
void* SKSE_CreateUIMessageData(RE::BSFixedString a_name)
{
    REL::Relocation<void* (*)(RE::BSFixedString)> func{RELOCATION_ID(22825, 35855)};
    return func(a_name);
}

// Focus world map view on target location
void FocusOnMapMarker(RE::TESObjectREFR* markerRef)
{
    // Stop if the map is not open
    if (!RE::UI::GetSingleton()->IsMenuOpen(RE::MapMenu::MENU_NAME)) return;

    /*
        // Check if menu is NOT open, and if so then open it
        if (RE::UI::GetSingleton()->IsMenuOpen(RE::MapMenu::MENU_NAME) == false) {
        MenuInteraction(MenuType::Map, MenuAction::Open);
        while (RE::UI::GetSingleton()->IsMenuOpen(RE::MapMenu::MENU_NAME) == false)
            Sleep(1250); // Brief pause for map to open
    }*/

    if (auto uiMessageQueue = RE::UIMessageQueue::GetSingleton()) {
        RefHandleUIData* data = static_cast<RefHandleUIData*>(SKSE_CreateUIMessageData(RE::InterfaceStrings::GetSingleton()->refHandleUIData));
        data->refHandle = markerRef->GetHandle().native_handle();
        uiMessageQueue->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kUpdate, data);
    }
}

// Navigate to target location (marker) on world map
void NavigateToLocation(std::string targetLocation)
{
    RE::TESObjectREFR* mapMarkerRef = IsLocationKnown(targetLocation);
    if (mapMarkerRef != NULL) {
        std::string result = "Navigating to " + targetLocation;
        SendNotification("Location: " + Title_Case(targetLocation));
        logger::info("{}", result);
        FocusOnMapMarker(mapMarkerRef);
    }
    else {
        std::string result = targetLocation + " location is not known";
        SendNotification(result);
        logger::info("{}", result);
    }
}

// Navigate to player's position on world map
void NavigateToPlayer()
{
    // Spoof button input to navigate to player's position on world map
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
        auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "playerPosition", 0, 1.0f, 0.0f);
        bsInputEventQueue->PushOntoInputQueue(kEvent);
        SendNotification("Location: Player");
    }

    /* // Also works
    auto playerRef = static_cast<RE::TESObjectREFR*>(player);
    FocusOnMapMarker(playerRef);
    SendNotification("Location: Player"); */
}

/*
void NavigateToCurrentQuest()
{
    // https://discord.com/channels/535508975626747927/535530099475480596/1096307972902244423

    if (!player) {
        return;
    }

    // Doesn't currently work on Clib-NG for VR (5/17/23)
    RE::BSSpinLockGuard TargetLock{player->GetPlayerRuntimeData().questTargetsLock};
    for (auto& [quest, targets] : player.questTargets) {
        if (!quest || !targets) {
            continue;
        }

        if (!quest->IsActive()) {
            continue;
        }

        for (auto target : *targets) {
            if (!target) {
                continue;
            }

            RE::BSWriteLockGuard AliasLock{quest->aliasAccessLock};
            // either
            {
                auto handle = quest->refAliasMap.find(target->alias);
                if (handle != quest->refAliasMap.end()) {
                    handle->second;  // ObjectRefHandle
                }
            }
            // or, I'm not sure
            {
                if (quest->aliases.size() < target->alias) {
                    if (auto baseAlias = quest->aliases[target->alias]) {
                        auto handle = quest->refAliasMap.find(baseAlias->aliasID);
                        if (handle != quest->refAliasMap.end()) {
                            handle->second;  // ObjectRefHandle
                        }
                    }
                }
            }
        }
    }
} */

// Place custom player marker when looking at map (or open UI dialog to manipulate an existing player marker)
void PlaceCustomMarker() {
    // Spoof button input to navigate to player's position on world map
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton()) {
        auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "placePlayerMarker", 0, 1.0f, 0.0f);
        bsInputEventQueue->PushOntoInputQueue(kEvent);
        SendNotification("Player Marker Placed");
    }
}

/*
// void PlaceMarker() {
//     REL::Relocation<void (*)()> func{RELOCATION_ID(52226, 53113)};
//     return func();
// }

// void PlaceMarker() {
//     using func_t = decltype(&PlaceMarker);
//     REL::Relocation<func_t> func{RELOCATION_ID(52226, 53113)};
//     return func();
// }
*/

#pragma endregion Control and interact with the map and map navigation

#pragma endregion Control and interact with menus



#pragma region Potentially Useful Commands for the future
//RE::PlayerCharacter::GetSingleton()->DrinkPotion

#pragma endregion

#pragma region Archive

// void TypeLocation(const std::string& word) {
//     /*SetForegroundWindow(windowHandle);
//     SetFocus(windowHandle);*/
//     int characterPause = 50;
//
//     // Press "F" key virtually to show Location Filter GUI
//     INPUT input = {};
//     input.type = INPUT_KEYBOARD;
//     input.ki.wVk = 'F';
//     input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
//     input.ki.dwFlags = 0;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(characterPause);
//     input.ki.dwFlags = KEYEVENTF_KEYUP;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(characterPause);
//
//     // Press "Space" key virtually to focus on location filter text box
//     input = {};
//     input.type = INPUT_KEYBOARD;
//     input.ki.wVk = VK_SPACE;
//     input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
//     input.ki.dwFlags = 0;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(characterPause);
//     input.ki.dwFlags = KEYEVENTF_KEYUP;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(characterPause);
//
//     // Iterate over each character in the word and press corresponding key virtually
//     for (char c : word) {
//         input = {};
//         input.type = INPUT_KEYBOARD;
//         input.ki.wVk = 0;
//         input.ki.wScan = MapVirtualKey(VkKeyScan(c), MAPVK_VK_TO_VSC);
//         input.ki.dwFlags = 0;
//         SendInput(1, &input, sizeof(INPUT));
//         Sleep(characterPause);
//         input.ki.dwFlags = KEYEVENTF_KEYUP;
//         SendInput(1, &input, sizeof(INPUT));
//         Sleep(characterPause);
//     }
//
//     Sleep(200);
//
//     // Press "Return" key virtually to confirm search criteria
//     input = {};
//     input.type = INPUT_KEYBOARD;
//     /// input.ki.wVk = VK_RETURN;
//     input.ki.wScan = MapVirtualKey(VkKeyScan(VK_RETURN), MAPVK_VK_TO_VSC);
//     input.ki.dwFlags = 0;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(200);
//     input.ki.dwFlags = KEYEVENTF_KEYUP;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(200);
//
//     for (int i = 0; i < 2; i++) {
//         // For some reason Skyrim doesn't like virtual "down arrow" input on the world map
//         input = {};
//         input.type = INPUT_KEYBOARD;
//         input.ki.wVk = VK_DOWN;  // Set virtual key code for down arrow key
//         input.ki.wScan = 0;
//         input.ki.time = 0;
//         input.ki.dwExtraInfo = 0;
//         input.ki.dwFlags = 0;                 // Set keydown flag
//         SendInput(1, &input, sizeof(INPUT));  // Send input
//         Sleep(300);
//         input.ki.dwFlags = KEYEVENTF_KEYUP;   // Set keyup flag
//         SendInput(1, &input, sizeof(INPUT));  // Send input
//         Sleep(300);
//     }
//
//     // Press "Return" key virtually to execute finding of target location
//     input = {};
//     input.type = INPUT_KEYBOARD;
//     input.ki.wVk = VK_RETURN;
//     /// input.ki.wScan = MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
//     input.ki.dwFlags = 0;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(200);
//     input.ki.dwFlags = KEYEVENTF_KEYUP;
//     SendInput(1, &input, sizeof(INPUT));
//     Sleep(200);
// }

// #include <cmath>
//
// void FocusOnMapMarker(RE::TESObjectREFR* markerRef) {
//     if (RE::UI::GetSingleton()->IsMenuOpen(RE::MapMenu::MENU_NAME) == true) {
//         if (auto uiMessageQueue = RE::UIMessageQueue::GetSingleton()) {
//
//             // Press "E" key virtually to show Location Filter GUI
//             INPUT input = {};
//             input.type = INPUT_KEYBOARD;
//             input.ki.wVk = 'E';
//             input.ki.wScan = MapVirtualKey(input.ki.wVk, MAPVK_VK_TO_VSC);
//             input.ki.dwFlags = 0;
//             SendInput(1, &input, sizeof(INPUT));
//             Sleep(50);
//             input.ki.dwFlags = KEYEVENTF_KEYUP;
//             SendInput(1, &input, sizeof(INPUT));
//             Sleep(1100);
//
//             // Get worldspace coordinates for actor
//             auto playerXPosition = (int)floor(actor->GetPosition().x);
//             auto playerYPosition = (int)floor(actor->GetPosition().y);
//             auto playerZPosition = (int)floor(actor->GetPosition().z);
//             std::string playerLocation = "Player = " + std::to_string(playerXPosition) + "," + std::to_string(playerYPosition) + "," + std::to_string(playerZPosition);
//             SendNotification(playerLocation.c_str());
//             logger::debug("{}", playerLocation);
//
//             // Get worldspace coordinates for location of interest
//             auto locationXPosition = (int)floor(markerRef->GetPositionX());
//             auto locationYPosition = (int)floor(markerRef->GetPositionY());
//             auto locationZPosition = (int)floor(markerRef->GetPositionZ());
//             std::string locationCoordinates = "Marker = " + std::to_string(locationXPosition) + "," + std::to_string(locationYPosition) + "," +
//             std::to_string(locationZPosition); SendNotification(locationCoordinates.c_str()); logger::debug("{}", locationCoordinates);
//
//
//             int xOffset = (locationXPosition - playerXPosition);
//             int yOffset = (locationYPosition - playerYPosition);
//             int zOffset = (locationZPosition - playerZPosition);
//             std::string translateValues = "Translate = " + std::to_string(xOffset) + "," + std::to_string(yOffset) + "," + std::to_string(zOffset);
//             SendNotification(translateValues.c_str());
//             logger::debug("{}", translateValues);
//
//             // Move map camera by inputted amount relative to actor position
//             auto scaler = 0.15/1000;
//             const auto mapMenu = RE::UI::GetSingleton()->GetMenu<RE::MapMenu>().get();
//             float cameraXOffset = xOffset * scaler;
//             float cameraYOffset = yOffset * scaler;
//             float cameraZOffset = zOffset * scaler;
//             mapMenu->GetRuntimeData2().camera.translationInput.x = cameraXOffset;
//             mapMenu->GetRuntimeData2().camera.translationInput.y = cameraYOffset;
//             std::string cameraAdjust = "Camera = " + std::to_string(cameraXOffset) + "," + std::to_string(cameraYOffset) + "," + std::to_string(cameraZOffset);
//             SendNotification(cameraAdjust.c_str());
//             logger::debug("{}", cameraAdjust);
//
//
//             const auto mapMenu = RE::UI::GetSingleton()->GetMenu<RE::MapMenu>().get();
//             mapMenu->GetRuntimeData2().camera.translationInput.x = cameraXOffset;
//
//             ///mapMenu->GetRuntimeData2().camera.translationInput.z = zOffset;
//
//             ///RE::ExtraMapMarker* marker = markerRef ? markerRef->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
//             ///RE::IUIMessageData* data = uiMessageQueue->CreateUIMessageData(RE::InterfaceStrings::GetSingleton()->refHandleUIData);
//
//             /*const auto mapMenu = RE::UI::GetSingleton()->GetMenu<RE::MapMenu>().get();
//             ///auto test1 = mapMenu->GetRuntimeData2().worldSpace->fixedCenter.x;
//
//             auto test2 = mapMenu->GetRuntimeData2().camera.;
//             std::string mapCenter = "Map = " + std::to_string(test2.x) + "," + std::to_string(test2.y);
//             SendNotification(mapCenter.c_str());
//             logger::debug("{}", mapCenter);*/
//
//             //auto mapRT1 = mapMenu->GetRuntimeData();
//
//             //// Rotate map camera by inputted degrees (left/right)
//             //mapMenu->GetRuntimeData2().camera.rotationInput.x = 60;
//
//             //// Rotate map camera by inputted degrees (up/down)
//             //mapMenu->GetRuntimeData2().camera.rotationInput.y = 30;
//
//             //// Zoom map camera by percentage (1 = 100% full zoom in, -1 = -100% full zoom out)
//             //mapMenu->GetRuntimeData2().camera.zoomInput = -1;
//
//             //// Move map camera by inputted amount relative to actor position
//             //mapMenu->GetRuntimeData2().camera.translationInput.x = 10000;
//             //mapMenu->GetRuntimeData2().camera.translationInput.y = 10000;
//             //mapMenu->GetRuntimeData2().camera.translationInput.z = 10000;
//
//             ///mapMenu->GetRuntimeData2().camera.worldSpace.;
//
//
//
//
//             ///const auto marker = markerRef ? markerRef->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
//
//
//              ///auto test = mapRT1.mapMarker.get().get();
//              /*auto test = mapRT2.worldSpace->fullName.c_str();
//              SendNotification(test);*/
//
//              ///auto test2 = mapRT2.worldSpace->worldMapData.cameraData.maxHeight;
//
//
//
//              /*///data->refHandle = markerRef->GetHandle().native_handle();
//
//              RE::RefHandle test;
//
//              RE::CreateRefHandle(test, markerRef);
//
//              ///data->pad0C = markerRef->GetHandle().native_handle();
//
//              data->pad0C = test;
//              uiMessageQueue->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kShow, data);*/
//          }
//      }
//  }

// inline RE::TESObjectREFR* GetMenuContainer(RE::StaticFunctionTag*) {
//     RE::TESObjectREFR* container = nullptr;
//
//     const auto UI = RE::UI::GetSingleton();
//     const auto menu = UI ? UI->GetMenu<RE::ContainerMenu>() : nullptr;
//     if (menu) {
//         const auto refHandle = menu->GetTargetRefHandle();
//         RE::TESObjectREFRPtr refr;
//         RE::LookupReferenceByHandle(refHandle, refr);
//
//         container = refr.get();
//     }

//// Player's last recorded Mount
//struct Mount
//{
//    static const int None = 0;
//    static const int Horse = RE::CameraState::kMount;
//    static const int Dragon = RE::CameraState::kDragon;
//};
#pragma endregion