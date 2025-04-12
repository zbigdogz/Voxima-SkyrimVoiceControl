#include "../functions/logger.hpp"  // SKSE log functions

RE::TESForm* currentVoice;
RE::PlayerCharacter* player;

// Global Variables
#pragma region In-Game Global Variables
RE::TESGlobal* VOX_Enabled;             // Enables or Disables Voice Recognition
RE::TESGlobal* VOX_UpdateInterval;      // Decides the update interval for loop-based CheckUpdate()
RE::TESGlobal* VOX_CheckForUpdate;      // Forces the system to check for an update. Also checks for a change in microphone
RE::TESGlobal* VOX_PushToSpeakType;     // Determines the type of push to speak. [0] = None. [1] = Hold. [2] = Toggle. [3] = Vocal
RE::TESGlobal* VOX_PushToSpeakKeyCode;  // The keycode (mouse, keyboard, gamepad, or VR) of the push-to-speak button
RE::TESGlobal* VOX_AutoCastPowers;      // Enables or Disables always auto-casting powers
RE::TESGlobal* VOX_AutoCastShouts;      // Enables or Disables always auto-casting shouts
RE::TESGlobal* VOX_ShowLog;             // Enables or Disables the notification log in the top left corner
RE::TESGlobal* VOX_ShoutKey;            // Determines the shout key that is press when a shout/power is cast
RE::TESGlobal* VOX_LongAutoCast;        // Enables or Disables casting spells that have a long casting time (like Flame Thrall)
RE::TESGlobal* VOX_Sensitivity;         // Determines how sensitive the voice recognition is
RE::TESGlobal*
    VOX_KnownShoutWordsOnly;  // Determines if you only get voice commands for shout words that are known, instead of all words after the first is learned
RE::TESGlobal* VOX_QuickLoadType;       //Determines whether there is a confirmation menu when quick loading

std::string openMenu;
#pragma endregion

#pragma region Enumerations and Structs
enum ActorSlot { Left, Right, Both, Voice, None };
enum MagicType { null, Spell, Power, Shout };
enum MenuType { Console, Favorites, Inventory, Journal, LevelUp, Magic, Map, Skills, SleepWait, Tween };
enum MenuAction { Open, Close };
enum MoveType {
    MoveForward,
    MoveLeft,
    MoveRight,
    TurnAround,
    TurnLeft,
    TurnRight,
    MoveJump,
    MoveWalk,
    MoveRun,
    MoveSprint,
    StopSprint,
    StopMoving
};  // Horse Controls

// Pre-made messages that the App is prepared to recieve
struct WebSocketMessage
{
    static constexpr const char* EnableRecognition = "enable recognition";
    static constexpr const char* DisableRecognition = "disable recognition";

    static constexpr const char* CheckForMicChange = "check for mic change";
    static constexpr const char* InitializeUpdate = "initialize update";
    static constexpr const char* UpdateConfiguration = "update configuration\n";

    static constexpr const char* UpdateSpells = "update spells\n";
    static constexpr const char* UpdatePowers = "update powers\n";
    static constexpr const char* UpdateShouts = "update shouts\n";

    static constexpr const char* UpdateLocations = "update locations\n";
    static constexpr const char* EnableLocationCommands = "enable location commands";
    static constexpr const char* DisableLocationCommands = "disable location commands";
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
static int ToSkyrimKeyCode(std::string keyString);
bool IsKeyDown(int keyCode);
static void SendKeyDown(int keycode);
static void SendKeyUp(int keycode);
void PressKey(int keycode, int milliseconds = 0);
void ToggleKey(int keycode, int milliseconds = 0);
static void MoveHorse(MoveType moveType);
void CompileAndRun(RE::Script* script, RE::TESObjectREFR* targetRef, RE::COMPILER_NAME name = RE::COMPILER_NAME::kSystemWindowCompiler);
void CompileAndRunImpl(RE::Script* script, RE::ScriptCompiler* compiler, RE::COMPILER_NAME name, RE::TESObjectREFR* targetRef);
RE::ObjectRefHandle GetSelectedRefHandle();
RE::NiPointer<RE::TESObjectREFR> GetSelectedRef();
void ExecuteConsoleCommand(std::vector<std::string> command);
static void MovePlayerHorse(RE::ActorPtr horse, int angle = -1);
static bool IsActorWalking(RE::ActorPtr actor);
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

    if (HWND windowHandle = FindWindow(NULL, (LPWSTR)(wchar_t*)std::wstring(buffer, buffer + strlen(buffer) + 1).c_str()))
    {
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

    if (player->GetMount(mount))
    {
        mountName = mount->GetRace()->GetName();

        if (mountName == "Horse")
        {
            logger::info("Riding Horse");
            return 1;
        }

        if (mountName == "Dragon Race")
        {
            logger::info("Riding Dragon");
            return 2;
        }
    }

    logger::info("Riding Nothing");
    return 0;
}

RE::ObjectRefHandle GetSelectedRefHandle()
{
    REL::Relocation<RE::ObjectRefHandle*> selectedRef{RELOCATION_ID(519394, REL::Module::get().version().patch() < 1130 ? 405935 : 504099)};
    return *selectedRef;
}

RE::NiPointer<RE::TESObjectREFR> GetSelectedRef()
{
    auto handle = GetSelectedRefHandle();
    return handle.get();
}

void CompileAndRunImpl(RE::Script* script, RE::ScriptCompiler* compiler, RE::COMPILER_NAME name, RE::TESObjectREFR* targetRef)
{
    using func_t = decltype(CompileAndRunImpl);
    REL::Relocation<func_t> func{RELOCATION_ID(21416, REL::Module::get().version().patch() < 1130 ? 21890 : 441582)};
    return func(script, compiler, name, targetRef);
}

void CompileAndRun(RE::Script* script, RE::TESObjectREFR* targetRef, RE::COMPILER_NAME name)
{
    RE::ScriptCompiler compiler;
    CompileAndRunImpl(script, &compiler, name, targetRef);
}

// Execute Console Commands
void ExecuteConsoleCommand(std::vector<std::string> command)
{
    // Some of the code here is from Valhalla Combat
    const auto scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
    const auto script = scriptFactory ? scriptFactory->Create() : nullptr;
    if (script)
    {
        //const auto selectedRef = RE::Console::GetSelectedRef(); // The console object
        
        const auto selectedRef = GetSelectedRef();

        for (std::string item : command)
        {
            // Check for special commands
            if (item.starts_with("wait"))
            {
                if (item.ends_with('m'))
                {
                    std::this_thread::sleep_for(std::chrono::minutes(std::stoi(item.replace(0, 4, "").replace(item.length() - 1, std::string::npos, ""))));
                }
                else if (item.ends_with('s'))
                {
                    std::this_thread::sleep_for(std::chrono::seconds(std::stoi(item.replace(0, 4, "").replace(item.length() - 1, std::string::npos, ""))));
                }
                else if (item.ends_with("ms"))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(item.replace(0, 4, "").replace(item.length() - 2, std::string::npos, ""))));
                }
            }
            else if (item.starts_with("press"))
            {
                int key = 0;
                std::string durationString = "";
                int duration = 0;
                int i = 0;

                // Iterate the string in reverse order to find the duration, if it exists
                for (i = item.length() - 1; i >= 0; --i)
                {
                    if (std::isdigit(item[i]))
                    {
                        // Add the digit character to the beginning of the number string
                        durationString.insert(0, 1, item[i]);
                    }
                    else if (!durationString.empty())
                    {
                        // Stop iterating when a non-digit character is encountered after extracting the number
                        break;
                    }
                }

                if (!durationString.empty())
                {
                    // Extract the key portion of the string
                    std::string keyString = item.substr(6, item.length() - (durationString.length() + 8));

                    if (item.ends_with("ms"))
                    {
                        duration = std::stoi(durationString);
                    }
                    else if (item.ends_with("s"))
                    {
                        duration = std::stoi(durationString) * 1000;
                    }
                    else if (item.ends_with("m"))
                    {
                        duration = std::stoi(durationString) * 1000 * 60;
                    }
                    else
                    {
                        duration = std::stoi(durationString);
                    }

                    key = ToSkyrimKeyCode(keyString);
                }
                else
                {
                    // No duration found, extract the key from the string
                    std::string keyString = item.substr(6);
                    key = ToSkyrimKeyCode(keyString);
                }

                PressKey(key, duration);
            }
            else if (item.starts_with("hold"))
            {
                SendKeyDown(ToSkyrimKeyCode(item.replace(0, 5, "")));
            }
            else if (item.starts_with("release"))
            {
                SendKeyUp(ToSkyrimKeyCode(item.replace(0, 8, "")));
            }
            else
            {
                logger::debug("Executing console command '{}'", item);
                script->SetCommand(item); // Set the command to `item`
                //script->CompileAndRun(selectedRef.get()); // Compile and run the console command
                CompileAndRun(script, selectedRef.get());
            } // End if/else
        }  // End for
    } // End if

    delete script;
}  // End ExecuteConsoleCommand

// Sends a notification to the top left in Skyrim, if the actor has logs enabled
void SendNotification(std::string message)
{
    if (VOX_ShowLog->value == 1)
    {
        RE::DebugNotification(message.c_str());
    }
}

#pragma region Horse Controls
// Command horse to execute a given movement type
static void MoveHorse(MoveType moveType)
{
    // Overview of how the horse is moved for VR
    /*   Keyboard movement in VR is based on world direction, not player direction.
     *   When "W" is recieved, the horseDirection will be set to the closest 8-degree direction that the horse is facing.
     *   All other commands will adjust the direction.
     *   When the horse stops moving, the direction will be recet to stationary.
     *   If any user input is recieved, the direction is reset to stationary.
     */

    std::thread([moveType]() {
        int delay = 500;
        int numIntervals = 50;
        int attempts = 4;
        RE::ActorPtr horse;
        bool move = false;

        if (player == nullptr) return;

        (void)player->GetMount(horse);

        if (horse == nullptr)
        {
            SendNotification("No Mount Detected");
            return;
        }

        // Movements that are the same in Flatrim and VR
        switch (moveType)
        {
            case MoveType::MoveJump:
                PressKey(ToSkyrimKeyCode("Space"));
                break;

            case MoveType::StopMoving:
                SendKeyUp(ToSkyrimKeyCode("W"));  // Direction (N)
                SendKeyUp(ToSkyrimKeyCode("D"));  // Direction (E)
                SendKeyUp(ToSkyrimKeyCode("S"));  // Direction (S)
                SendKeyUp(ToSkyrimKeyCode("A"));  // Direction (W)

                SendKeyUp(ToSkyrimKeyCode("LAlt"));  // LAlt (Toggles walking/running)
                break;

            case MoveType::MoveSprint:
                do
                {
                    if (!horse->AsActorState()->IsSprinting())
                    {
                        if (!horse->IsMoving())
                        {
                            MovePlayerHorse(horse);
                            Sleep(500);
                        }

                        if (IsActorWalking(horse))
                        {
                            ToggleKey(ToSkyrimKeyCode("LAlt"));
                            Sleep(500);
                        }

                        if (!horse->AsActorState()->IsSprinting())
                        {
                            PressKey(ToSkyrimKeyCode("LShift"));
                        }
                    }

                    for (int i = numIntervals; !horse->AsActorState()->IsSprinting() && i > 0; i--)
                    {
                        Sleep(delay);
                    }

                    attempts--;
                    if (attempts == 0) break;
                } while (!horse->AsActorState()->IsSprinting());
                break;

            case MoveType::StopSprint:
                if (horse->AsActorState()->IsSprinting()) PressKey(ToSkyrimKeyCode("LShift"));
                break;

            case MoveType::MoveRun:
                do
                {
                    // If sprinting
                    if (horse->AsActorState()->IsSprinting())
                    {
                        PressKey(ToSkyrimKeyCode("LShift"));
                        Sleep(500);
                    }
                    // If not moving
                    else if (!horse->IsMoving())
                    {
                        MovePlayerHorse(horse);
                        Sleep(500);
                    }

                    if (IsActorWalking(horse))
                    {
                        ToggleKey(ToSkyrimKeyCode("LAlt"));
                    }

                    for (int i = numIntervals; !horse->IsRunning() && i > 0; i--)
                    {
                        Sleep(delay);
                    }

                    attempts--;
                    if (attempts == 0) break;
                } while (!horse->IsRunning());
                break;

            case MoveType::MoveWalk:
                do
                {
                    // If they are sprinting, stop sprinting
                    if (horse->AsActorState()->IsSprinting())
                    {
                        PressKey(ToSkyrimKeyCode("LShift"));
                        Sleep(500);
                    }
                    // If they are not moving, make them move
                    else if (!horse->IsMoving())
                    {
                        MovePlayerHorse(horse);
                        Sleep(500);
                    }

                    // If they are running, have them walk
                    if (horse->IsRunning())
                    {
                        ToggleKey(ToSkyrimKeyCode("LAlt"));
                    }

                    for (int i = numIntervals; !IsActorWalking(horse) && i > 0; i--)
                    {
                        Sleep(delay);
                    }

                    attempts--;
                    if (attempts == 0) break;
                } while (!IsActorWalking(horse));
                break;
        }

        // VR-specific control
        if (REL::Module::IsVR())
        {
            // Get Facing
            float angleZ = horse->GetAngleZ();

            // Adjust the angle to a 360 degree metric, from a 6.3 degree metric
            angleZ = (float)(angleZ * 360 / 6.3);

            // Adjust the angle to 8-degrees of freedom, from 360-degrees of freedom
            angleZ = roundf(angleZ / 45) * 45;

            // Adjust the horse as needed
            switch (moveType)
            {
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

                case MoveType::TurnAround:
                    move = true;
                    // Rotate "HorseDirection" by 180
                    angleZ += 180;

                    if (angleZ > 360) angleZ -= 360;
                    break;
            }

            // Move the horse in desired direction
            if (move == true)
            {
                MovePlayerHorse(horse, (int)angleZ);
            }
        }
        else
        {
            // Player SE or AE
            switch (moveType)
            {
                case MoveType::MoveForward:
                    MovePlayerHorse(horse);
                    break;

                    // Disabled, since they don't look good on SE

                    /*
                case MoveType::TurnRight:
                    SendKeyDown(ToSkyrimKeyCode("W);  // Direction (N)
                    SendKeyDown(ToSkyrimKeyCode("D);  // Direction (E)
                    SendKeyUp(ToSkyrimKeyCode("S);    // Direction (S)
                    SendKeyUp(ToSkyrimKeyCode("A);    // Direction (W)
                    break;

                case MoveType::MoveRight:
                    SendKeyUp(ToSkyrimKeyCode("W);    // Direction (N)
                    SendKeyDown(ToSkyrimKeyCode("D);  // Direction (E)
                    SendKeyUp(ToSkyrimKeyCode("S);    // Direction (S)
                    SendKeyUp(ToSkyrimKeyCode("A);    // Direction (W)
                    break;

                case MoveType::TurnAround:
                    SendKeyUp(ToSkyrimKeyCode("W);    // Direction (N)
                    SendKeyUp(ToSkyrimKeyCode("D);    // Direction (E)
                    SendKeyDown(ToSkyrimKeyCode("S);  // Direction (S)
                    SendKeyUp(ToSkyrimKeyCode("A);    // Direction (W)
                    break;

                case MoveType::MoveLeft:
                    SendKeyUp(ToSkyrimKeyCode("W);    // Direction (N)
                    SendKeyUp(ToSkyrimKeyCode("D);    // Direction (E)
                    SendKeyUp(ToSkyrimKeyCode("S);    // Direction (S)
                    SendKeyDown(ToSkyrimKeyCode("A);  // Direction (W)
                    break;

                case MoveType::TurnLeft:
                    SendKeyDown(ToSkyrimKeyCode("W);  // Direction (N)
                    SendKeyUp(ToSkyrimKeyCode("D);    // Direction (E)
                    SendKeyUp(ToSkyrimKeyCode("S);    // Direction (S)
                    SendKeyDown(ToSkyrimKeyCode("A);  // Direction (W)
                    break;
                    */
            }
        }
    }).detach();

    /*
     * Potential Alternatives for the future:
     *   It's possible that moving the mouse will rotate the horse, causing "W" to move the horse in the new direction
     *   I may be able to use the VR controller's joystick to move just the horse. Maybe the code that move sthe horse in the actor's direction isn't directly
     * tied to the movement of the joystick Maybe a gamepad joystick can be simulated. It's possible that hte game will recognize the VR controllers as
     * joysticks RE the code for WASD to see what exactly is happening when you press those keys. Perhaps I can manually do it, and move in any direction I want
     */
}

// Moves the player's horse forward (or in the given direction for VR)
static void MovePlayerHorse(RE::ActorPtr horse, int angle)
{
    int newAngle = angle;

    if (REL::Module::IsVR())
    {
        // If the angle is not provided, get the current angle
        if (angle == -1)
        {
            // Get Facing
            newAngle = horse->GetAngleZ();

            // Adjust the angle to a 360 degree metric, from a 6.3 degree metric
            newAngle = (float)(newAngle * 360 / 6.3);

            // Adjust the angle to 8-degrees of freedom, from 360-degrees of freedom
            newAngle = roundf(newAngle / 45) * 45;
        }

        // Move the horse in desired direction
        switch ((int)newAngle)
        {
            case 0:
            case 360:
                SendKeyDown(ToSkyrimKeyCode("W"));  // Direction (N)
                SendKeyUp(ToSkyrimKeyCode("D"));    // Direction (E)
                SendKeyUp(ToSkyrimKeyCode("S"));    // Direction (S)
                SendKeyUp(ToSkyrimKeyCode("A"));    // Direction (W)
                break;

            case 45:
                SendKeyDown(ToSkyrimKeyCode("W"));  // Direction (N)
                SendKeyDown(ToSkyrimKeyCode("D"));  // Direction (E)
                SendKeyUp(ToSkyrimKeyCode("S"));    // Direction (S)
                SendKeyUp(ToSkyrimKeyCode("A"));    // Direction (W)
                break;

            case 90:
                SendKeyUp(ToSkyrimKeyCode("W"));    // Direction (N)
                SendKeyDown(ToSkyrimKeyCode("D"));  // Direction (E)
                SendKeyUp(ToSkyrimKeyCode("S"));    // Direction (S)
                SendKeyUp(ToSkyrimKeyCode("A"));    // Direction (W)
                break;

            case 135:
                SendKeyUp(ToSkyrimKeyCode("W"));    // Direction (N)
                SendKeyDown(ToSkyrimKeyCode("D"));  // Direction (E)
                SendKeyDown(ToSkyrimKeyCode("S"));  // Direction (S)
                SendKeyUp(ToSkyrimKeyCode("A"));    // Direction (W)
                break;

            case 180:
                SendKeyUp(ToSkyrimKeyCode("W"));    // Direction (N)
                SendKeyUp(ToSkyrimKeyCode("D"));    // Direction (E)
                SendKeyDown(ToSkyrimKeyCode("S"));  // Direction (S)
                SendKeyUp(ToSkyrimKeyCode("A"));    // Direction (W)
                break;

            case 225:
                SendKeyUp(ToSkyrimKeyCode("W"));    // Direction (N)
                SendKeyUp(ToSkyrimKeyCode("D"));    // Direction (E)
                SendKeyDown(ToSkyrimKeyCode("S"));  // Direction (S)
                SendKeyDown(ToSkyrimKeyCode("A"));  // Direction (W)
                break;

            case 270:
                SendKeyUp(ToSkyrimKeyCode("W"));    // Direction (N)
                SendKeyUp(ToSkyrimKeyCode("D"));    // Direction (E)
                SendKeyUp(ToSkyrimKeyCode("S"));    // Direction (S)
                SendKeyDown(ToSkyrimKeyCode("A"));  // Direction (W)
                break;

            case 315:
                SendKeyDown(ToSkyrimKeyCode("W"));  // Direction (N)
                SendKeyUp(ToSkyrimKeyCode("D"));    // Direction (E)
                SendKeyUp(ToSkyrimKeyCode("S"));    // Direction (S)
                SendKeyDown(ToSkyrimKeyCode("A"));  // Direction (W)
                break;
        }
    }
    else
    {
        SendKeyDown(ToSkyrimKeyCode("W"));  // Direction (N)
        SendKeyUp(ToSkyrimKeyCode("D"));    // Direction (E)
        SendKeyUp(ToSkyrimKeyCode("S"));    // Direction (S)
        SendKeyUp(ToSkyrimKeyCode("A"));    // Direction (W)
    }
}

// Returns if the actor is currently walking (Actor->IsWalking() returns true always)
static bool IsActorWalking(RE::ActorPtr actor)
{
    // Returns true if the actor is moving, but not running, sprinting, swimming, or in midair

    return actor->IsMoving() && !actor->IsRunning() && !actor->AsActorState()->IsSprinting() && !actor->AsActorState()->IsSwimming() && !actor->IsInMidair();
}
#pragma endregion

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

#pragma endregion Determines the players current morph(None, Werewolf, or Vampire Lord)

#pragma region Equip / Unequip / Casting
// Equip an item to an actor
void EquipToActor(RE::Actor* actor, RE::TESForm* item, ActorSlot hand, bool notification)
{
    try
    {
        /*
         * When inventory functionality is added, this function will be used to equip items/armor, too
         * Probably also for using potions/scrolls
         */

        std::vector<std::string> commands;
        std::string currentCommand;

        // Get Actor Process to check for currently equipped items
        RE::AIProcess* actorProcess = actor->GetActorRuntimeData().currentProcess;
        RE::TESForm* left_hand = actorProcess->GetEquippedLeftHand();
        RE::TESForm* right_hand = actorProcess->GetEquippedRightHand();
        RE::TESForm* voice = actor->GetActorRuntimeData().selectedPower;

        // If the item does not exist, i.e. a nullptr
        if (!item)
        {
            logger::error("ERROR::Item cannot be equipped. It is NULL");
            return;
        }

        if (item->GetKnown() || actor->HasSpell(item->As<RE::SpellItem>()))
        {
            //  Spell/Power
            if (item->As<RE::SpellItem>())
            {
                RE::SpellItem* spell = item->As<RE::SpellItem>();
                std::string spellName = spell->GetFullName();

                switch (hand)
                {
                    case ActorSlot::Left:
                        if (left_hand == nullptr || left_hand != nullptr && spell != left_hand->As<RE::SpellItem>())
                        {
                            RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.leftHand());
                        }
                        else
                        {
                            logger::debug("Item already equipped to the Left Hand");
                        }

                        if (notification) SendNotification("Equipping: " + spellName + " Left");
                        break;

                    case ActorSlot::Right:
                        if (right_hand == nullptr || right_hand != nullptr && spell != right_hand->As<RE::SpellItem>())
                        {
                            RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.rightHand());
                        }
                        else
                        {
                            logger::debug("Item already equipped to the Right Hand");
                        }

                        if (notification) SendNotification("Equipping: " + spellName + " Right");
                        break;

                    case ActorSlot::Both:
                        if (left_hand == nullptr || left_hand != nullptr && spell != left_hand->As<RE::SpellItem>())
                        {
                            RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.leftHand());
                        }
                        else
                        {
                            logger::debug("Item already equipped to the Left Hand");
                        }

                        if (right_hand == nullptr || right_hand != nullptr && spell != right_hand->As<RE::SpellItem>())
                        {
                            RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.rightHand());
                        }
                        else
                        {
                            logger::debug("Item already equipped to the Right Hand");
                        }

                        if (notification) SendNotification("Equipping: " + spellName + " Both");
                        break;

                    case ActorSlot::Voice:
                        if (voice == nullptr || voice != nullptr && spell != voice->As<RE::SpellItem>())
                            RE::ActorEquipManager::GetSingleton()->EquipSpell(actor, spell, actorSlot.voice());

                        if (notification) SendNotification("Equipping Power: " + spellName);
                        break;
                }
            }
            // Shout
            else if (item->As<RE::TESShout>())
            {
                RE::TESShout* shout = item->As<RE::TESShout>();
                std::string shoutName = shout->GetFullName();

                if (voice == nullptr || voice != nullptr && shout != voice->As<RE::TESShout>()) RE::ActorEquipManager::GetSingleton()->EquipShout(actor, shout);

                if (notification) SendNotification("Equipping Shout: " + shoutName);
            }
        }
        else
        {
            logger::info("Item not equipped. Player does not know it");
        }  // End check if actor knows item

        // logger::info("Item Equipped");
    }
    catch (const std::exception& ex)
    {
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

    switch (hand)
    {
        case ActorSlot::Left:
        case ActorSlot::Right:
        case ActorSlot::Both:
            if (left_hand)
                if (left_hand->Is(RE::FormType::Spell))
                    un_equip_spell(nullptr, 0, actor, left_hand->As<RE::SpellItem>(), 0);
                else
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(actor, left_hand->As<RE::TESBoundObject>(), nullptr, 1U, actorSlot.leftHand(), false,
                                                                         false, unEquipSound);

            if (right_hand)
                if (right_hand->Is(RE::FormType::Spell))
                    un_equip_spell(nullptr, 0, actor, right_hand->As<RE::SpellItem>(), 1);
                else
                    RE::ActorEquipManager::GetSingleton()->UnequipObject(actor, right_hand->As<RE::TESBoundObject>(), nullptr, 1U, actorSlot.rightHand(), false,
                                                                         false, unEquipSound);
            break;

        case ActorSlot::Voice:
            if (voice)
            {
                // shout
                if (voice->Is(RE::FormType::Shout))
                {
                    un_equip_shout(nullptr, 0, actor, voice->As<RE::TESShout>());
                }
                // Power
                else if (voice->Is(RE::FormType::Spell))
                {
                    un_equip_spell(nullptr, 0, actor, voice->As<RE::SpellItem>(), 2);
                }
            }
            break;
    }
}

// Cast Magic from actor slot. Returns whether the cast was successful
bool CastMagic(RE::Actor* actor, RE::TESForm* item, ActorSlot hand, int modifier = 0)
{
    // Modifier meaning
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
            std::string shoutWordName = item->As<RE::TESShout>()->variations[modifier].spell->fullName.c_str();  // Capture the target shout words
            currentVoice = actor->GetActorRuntimeData().selectedPower;                                           // Capture currently equipped shout/power
            EquipToActor(actor, item, ActorSlot::Voice, false);                                                  // Equip target shout
            SendNotification("Casting Shout: " + shoutWordName);
            std::thread castShout(CastVoice, actor, item, modifier);  // Create new thread to cast shout via button spoofing
            castShout.detach();                                       // Run new thread

            /* // Directly casts the shout, but it doesn't trigger the dovazul voice or thunderclap sound
            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(item->As<RE::TESShout>()->variations[modifier].spell, false,
            nullptr, 1.0f, false, 0.0f, actor); */

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
        else if (item->As<RE::SpellItem>() && hand == ActorSlot::Voice)
        {
            logger::info("Casting Power");
            std::string spellName = item->As<RE::MagicItem>()->fullName.c_str();  // Capture the target power name
            currentVoice = actor->GetActorRuntimeData().selectedPower;            // Capture currently equipped shout/power
            EquipToActor(actor, item, ActorSlot::Voice, false);                   // Equip target power
            SendNotification("Casting Power: " + spellName);
            std::thread castShout(CastVoice, actor, item, modifier);  // Create new thread to cast power via button spoofing
            castShout.detach();                                       // Run new thread

            /* // Directly cast the power. However this bypasses in-game cooldown for powers
            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->CastSpellImmediate(item->As<RE::MagicItem>(), false, nullptr, 1.0f, false, 0.0f,
            actor); */

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
        else if (item->As<RE::SpellItem>())
        {
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
                        // SendNotification("Alteration");
                        break;

                    case RE::ActorValue::kConjuration:
                        accountForDualPerk = actor->HasPerk(dualConjurationPerk);
                        // SendNotification("Conjuration");
                        break;

                    case RE::ActorValue::kDestruction:
                        accountForDualPerk = actor->HasPerk(dualDestructionPerk);
                        // SendNotification("Destruction");
                        break;

                    case RE::ActorValue::kIllusion:
                        accountForDualPerk = actor->HasPerk(dualIllusionPerk);
                        // SendNotification("Illusion");
                        break;

                    case RE::ActorValue::kRestoration:
                        accountForDualPerk = actor->HasPerk(dualRestorationPerk);
                        // SendNotification("Restoration");
                        break;

                    default:
                        SendNotification("Unknown Skill Tree");
                        break;
                }
            }

            if (canCastSingle || canCastDual || IsGodMode())
            {
                // Damage Magicka
                if (accountForDualPerk)
                    actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -duelMagickaCost);
                else
                    actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -singleMagickaCost);

                switch (hand)
                {
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

                        // Disabled Dual Casting Perk
                        if (accountForDualPerk) actor->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand)->SetDualCasting(false);

                        return true;

                        break;

                    case ActorSlot::Both:
                        if (canCastDual || IsGodMode())
                        {
                            logger::info("Casting Both hands");
                            SendNotification("Casting: " + spellName + " Dual");

                            // Cast From Left Hand
                            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)
                                ->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, actor);

                            // Cast From Right Hand
                            actor->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand)
                                ->CastSpellImmediate(spell, false, nullptr, 1.0f, false, 0.0f, actor);

                            // Remove Magicka again
                            actor->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, -singleMagickaCost);

                            return true;
                        }
                        break;
                }  // End Switch
            }      // End Can Cast
        }          // End is valid item
    }              // End is known item
    else
    {
        logger::info("Item not cast. Player does not know it");
    }  // Determine if the actor knows the item

    return false;
}

// Asynconously casts a shout/power from the actor
void CastVoice(RE::Actor* actor, RE::TESForm* item, int level)
{
    // Spoof button input to cast shout/power
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton())
    {
        int holdTime = 10;
        if (item->As<RE::TESShout>())
        {
            switch (level)
            {
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
#pragma endregion Equipping, Unequipping, and Casting and items and magic

#pragma region Get Actor Items /Magic
// Retrieves known Shout and Word of Power data
std::vector<std::string> GetShoutList()
{
    std::vector<std::string> shoutList;
    auto playerSpells = player->GetActorBase()->GetSpellList();  // Obtain actor spell data
    RE::TESShout** playerShouts = playerSpells->shouts;          // Obtain all of actor's known shouts
    int numberOfShouts = (int)playerSpells->numShouts;           // Obtain number of actor's known shouts
    /// logger::debug("Number of Known Shouts = {}", numberOfShouts);
    try
    {
        for (int i = 0; i < numberOfShouts; i++)
        {                                                     // Loop through each of the actor's known shouts
            RE::TESShout* shout = playerShouts[i];            // Capture the current shout at index i
            const char* shoutName = shout->fullName.c_str();  // Capture the name of the shout
            shoutList.push_back(shoutName);                   // Add the shoutName to the shoutList (thereby growing the list size)
            shoutList[i] +=
                "\t" + std::format("{:X}", shout->GetLocalFormID()) + '\t' + "shout" + '\t' + shout->GetFile(0)->GetFilename().data();  // Add shout information
            /// logger::debug("Shout {} Name = {}", i + 1, shoutList[i]);
            for (int j = 0; j <= 2; j++)
            {                                                                 // Loop through all three shout words of power
                RE::TESWordOfPower* wordOfPower = shout->variations[j].word;  // Capture shout's word of power at j index
                if (wordOfPower && (VOX_KnownShoutWordsOnly->value == 0 || wordOfPower->formFlags & 0x10000))
                {  // Check if current word of power is "shoutable" by actor (both known AND unlocked) and if actor wants to shout only known words
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
    catch (const std::exception& ex)
    {
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

    switch (type1)
    {
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

    switch (type2)
    {
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

    switch (type3)
    {
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
    if (getSpells || getPowers)
    {
        int numSpells[2] = {};

        // Get spells from Player Base
        RE::SpellItem** baseSpells = actor->GetActorBase()->GetSpellList()->spells;  // List of the actor's spells.
        numSpells[0] = actor->GetActorBase()->GetSpellList()->numSpells;             // The number of spells the actor has.

        // Get Spells from PlayerCharacter (all but Base)
        RE::SpellItem** raceSpells = actor->GetRace()->actorEffects->spells;  // List of the actor's spells.
        numSpells[1] = actor->GetRace()->actorEffects->numSpells;             // The number of spells the actor has.

        // Base Spells/Powers
        for (int i = 0; i < numSpells[0]; i++)
        {
            RE::SpellItem* spell = baseSpells[i];
            std::string formID = "";

            if (spell->GetFile()->IsLight())
            {
                formID = std::format("{:X}", spell->GetFormID());
                formID = formID.substr(formID.length() - 6);  // Get last 6 characters
            }
            else
            {
                formID = std::format("{:X}", spell->GetLocalFormID());
            }

            switch (spell->GetSpellType())
            {
                case RE::MagicSystem::SpellType::kSpell:
                    if (getSpells)
                    {
                        updateSpells += (std::string)spell->GetName() + '\t' + formID + '\t' + "spell" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;

                case RE::MagicSystem::SpellType::kPower:
                case RE::MagicSystem::SpellType::kLesserPower:
                case RE::MagicSystem::SpellType::kVoicePower:
                    if (getPowers)
                    {
                        updatePowers += (std::string)spell->GetName() + '\t' + formID + '\t' + "power" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;
            }  // End Switch
        }      // End Base Spells/Powers

        // Racial Spells/Powers
        for (int i = 0; i < numSpells[1]; i++)
        {
            RE::SpellItem* spell = raceSpells[i];
            std::string formID = "";

            if (spell->GetFile()->IsLight())
            {
                formID = std::format("{:X}", spell->GetFormID());
                formID = formID.substr(formID.length() - 6);  // Get last 6 characters
            }
            else
            {
                formID = std::format("{:X}", spell->GetLocalFormID());
            }

            switch (spell->GetSpellType())
            {
                case RE::MagicSystem::SpellType::kSpell:
                    if (getSpells)
                    {
                        updateSpells += (std::string)spell->GetName() + '\t' + formID + '\t' + "spell" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;

                case RE::MagicSystem::SpellType::kPower:
                case RE::MagicSystem::SpellType::kLesserPower:
                case RE::MagicSystem::SpellType::kVoicePower:
                    if (getPowers)
                    {
                        updatePowers += (std::string)spell->GetName() + '\t' + formID + '\t' + "power" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;
            }  // End Switch
        }      // End Racial Spells/Powers

        // Obtained Spells/Powers
        for (auto& spell : actor->GetActorRuntimeData().addedSpells)
        {
            std::string formID = "";

            if (spell->GetFile()->IsLight())
            {
                formID = std::format("{:X}", spell->GetFormID());
                if (!formID.contains("FE"))
                {
                    formID = std::format("{:X}", spell->GetLocalFormID());
                }
                else
                {
                    formID = formID.substr(formID.length() - 6);  // Get last 6 characters
                }
            }
            else
            {
                formID = std::format("{:X}", spell->GetLocalFormID());
            }

            switch (spell->GetSpellType())
            {
                case RE::MagicSystem::SpellType::kSpell:
                    if (getSpells)
                    {
                        updateSpells += (std::string)spell->GetName() + '\t' + formID + '\t' + "spell" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;

                case RE::MagicSystem::SpellType::kPower:
                case RE::MagicSystem::SpellType::kLesserPower:
                case RE::MagicSystem::SpellType::kVoicePower:
                    if (getPowers)
                    {

                        updatePowers += (std::string)spell->GetName() + '\t' + formID + '\t' + "power" + '\t' +
                                        spell->GetFile(0)->GetFilename().data() + "\n";
                    }
                    break;
            }  // End Switch
        }      // End Obtained Spells/Powers
    }          // End if Spells or Powers

    // Owned Shouts
    if (getShouts)
    {
        for (std::string shout : GetShoutList())
        {
            updateShouts += shout + "\n";
        }

    }  // End Owned Shouts

    switch (type1)
    {
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

    switch (type2)
    {
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

    switch (type3)
    {
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

    for (std::string* item : l33tTranslation)
    {
        pos = 0;
        while ((pos = finalString.find(item[0], pos)) != std::string::npos)
        {
            finalString.replace(pos, item[0].length(), item[1]);
            pos += item[1].length();
        }
    }

    return finalString;
}
#pragma endregion Gets the players current Spells, Powers, and Shouts

#pragma region Key Output Management
// Turns a string value into its Skyrim Key code equivalent
static int ToSkyrimKeyCode(std::string keyString)
{
    // https://www.creationkit.com/index.php?title=Input_Script

    std::string keyUpper = keyString;
    std::transform(keyUpper.begin(), keyUpper.end(), keyUpper.begin(), [](unsigned char c) { return std::toupper(c); });

    if (keyUpper == "NONE")
        return 0;

    else if (keyUpper == "ESCAPE")
        return 1;

    else if (keyUpper == "1")
        return 2;

    else if (keyUpper == "2")
        return 3;

    else if (keyUpper == "3")
        return 4;

    else if (keyUpper == "4")
        return 5;

    else if (keyUpper == "5")
        return 6;

    else if (keyUpper == "6")
        return 7;

    else if (keyUpper == "7")
        return 8;

    else if (keyUpper == "8")
        return 9;

    else if (keyUpper == "9")
        return 10;

    else if (keyUpper == "0")
        return 11;

    else if (keyUpper == "-")
        return 12;

    else if (keyUpper == "=")
        return 13;

    else if (keyUpper == "BACKSPACE")
        return 14;

    else if (keyUpper == "TAB" || keyUpper == "\\t")
        return 15;

    else if (keyUpper == "Q")
        return 16;

    else if (keyUpper == "W")
        return 17;

    else if (keyUpper == "E")
        return 18;

    else if (keyUpper == "R")
        return 19;

    else if (keyUpper == "T")
        return 20;

    else if (keyUpper == "Y")
        return 21;

    else if (keyUpper == "U")
        return 22;

    else if (keyUpper == "I")
        return 23;

    else if (keyUpper == "O")
        return 24;

    else if (keyUpper == "P")
        return 25;

    else if (keyUpper == "[")
        return 26;

    else if (keyUpper == "]")
        return 27;

    else if (keyUpper == "ENTER")
        return 28;

    else if (keyUpper == "LCTRL")
        return 29;

    else if (keyUpper == "A")
        return 30;

    else if (keyUpper == "S")
        return 31;

    else if (keyUpper == "D")
        return 32;

    else if (keyUpper == "F")
        return 33;

    else if (keyUpper == "G")
        return 34;

    else if (keyUpper == "H")
        return 35;

    else if (keyUpper == "J")
        return 36;

    else if (keyUpper == "K")
        return 37;

    else if (keyUpper == "L")
        return 38;

    else if (keyUpper == ";")
        return 39;

    else if (keyUpper == "\"")
        return 40;

    else if (keyUpper == "~")
        return 41;

    else if (keyUpper == "LSHIFT")
        return 42;

    else if (keyUpper == "\\")
        return 43;

    else if (keyUpper == "Z")
        return 44;

    else if (keyUpper == "X")
        return 45;

    else if (keyUpper == "C")
        return 46;

    else if (keyUpper == "V")
        return 47;

    else if (keyUpper == "B")
        return 48;

    else if (keyUpper == "N")
        return 49;

    else if (keyUpper == "M")
        return 50;

    else if (keyUpper == ",")
        return 51;

    else if (keyUpper == ".")
        return 52;

    else if (keyUpper == "/")
        return 53;

    else if (keyUpper == "RSHIFT")
        return 54;

    else if (keyUpper == "LALT")
        return 56;

    else if (keyUpper == "SPACE")
        return 57;

    else if (keyUpper == "CAPS")
        return 58;

    else if (keyUpper == "F1")
        return 59;

    else if (keyUpper == "F2")
        return 60;

    else if (keyUpper == "F3")
        return 61;

    else if (keyUpper == "F4")
        return 62;

    else if (keyUpper == "F5")
        return 63;

    else if (keyUpper == "F6")
        return 64;

    else if (keyUpper == "F7")
        return 65;

    else if (keyUpper == "F8")
        return 66;

    else if (keyUpper == "F9")
        return 67;

    else if (keyUpper == "F10")
        return 68;

    else if (keyUpper == "F11")
        return 87;

    else if (keyUpper == "F12")
        return 88;

    else if (keyUpper == "RCTRL")
        return 157;

    else if (keyUpper == "RALT")
        return 184;

    else if (keyUpper == "UP")
        return 200;

    else if (keyUpper == "LEFT")
        return 203;

    else if (keyUpper == "RIGHT")
        return 205;

    else if (keyUpper == "END")
        return 207;

    else if (keyUpper == "DOWN")
        return 208;

    else if (keyUpper == "DELETE")
        return 211;

    else if (keyUpper == "MOUSE1")
        return 256;

    else if (keyUpper == "MOUSE2")
        return 257;

    else if (keyUpper == "MIDDLEMOUSE")
        return 258;

    else if (keyUpper == "MOUSE3")
        return 259;

    else if (keyUpper == "MOUSE4")
        return 260;

    else if (keyUpper == "MOUSE5")
        return 261;

    else if (keyUpper == "MOUSE6")
        return 262;

    else if (keyUpper == "MOUSE7")
        return 263;

    else if (keyUpper == "MOUSEWHEELUP")
        return 264;

    else if (keyUpper == "MOUSEWHEELDOWN")
        return 265;

    else if (keyUpper == "GAMEPAD DPAD_UP")
        return 266;

    else if (keyUpper == "GAMEPAD DPAD_DOWN")
        return 267;

    else if (keyUpper == "GAMEPAD DPAD_LEFT")
        return 268;

    else if (keyUpper == "GAMEPAD DPAD_RIGHT")
        return 269;

    else if (keyUpper == "GAMEPAD START")
        return 270;

    else if (keyUpper == "GAMEPAD BACK")
        return 271;

    else if (keyUpper == "GAMEPAD LEFT_THUMB")
        return 272;

    else if (keyUpper == "GAMEPAD RIGHT_THUMB")
        return 273;

    else if (keyUpper == "GAMEPAD LEFT_SHOULDER")
        return 274;

    else if (keyUpper == "GAMEPAD RIGHT_SHOULDER")
        return 275;

    else if (keyUpper == "GAMEPAD A")
        return 276;

    else if (keyUpper == "GAMEPAD B")
        return 277;

    else if (keyUpper == "GAMEPAD X")
        return 278;

    else if (keyUpper == "GAMEPAD Y")
        return 279;

    else if (keyUpper == "GAMEPAD LT")
        return 280;

    else if (keyUpper == "GAMEPAD RT")
        return 281;

    else if (keyUpper == "ALL")
        return -1;

    return 0;
}

// Briefly Press Key
void PressKey(int keycode, int milliseconds)
{
    SendKeyDown(keycode);
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    SendKeyUp(keycode);
}

// Toggles a Key for a given amount of time (0 for no toggle back)
void ToggleKey(int keycode, int milliseconds)
{
    //"milliseconds" refers to the amount of time before the key is returned to its initial state
    // by default, it's 0, meaning it will NOT be returned to its original state

    // If Key is down, send it up
    if (IsKeyDown(keycode))
    {
        SendNotification("Send Up");
        SendKeyUp(keycode);
    }
    // If Key is up, send it down
    else
    {
        SendNotification("Send Down");
        SendKeyDown(keycode);
    }

    if (milliseconds > 0)
    {
        std::thread([keycode, milliseconds]() {
            Sleep(milliseconds);
            ToggleKey(keycode);
        }).detach();
    }
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

// Returns whether the specified keyboard, mouse, or VR key is held down (only keyboard works. CommonLib issue)
bool IsKeyDown(int keyCode)
{
    uint32_t button = (uint32_t)keyCode;

    int keyboardRange[2] = {0, 255};
    int mouseRange[2] = {256, 265};

    int mouseOffset = 256;
    int vrRightOffset = 410;
    int vrLeftOffset = 474;
    // int gamepadOffset = null;

    auto inputManager = RE::BSInputDeviceManager::GetSingleton();
    RE::BSInputDevice* device = nullptr;

    if (button <= keyboardRange[1])
    {
        return (inputManager->GetKeyboard()->curState[button] & 0x80) != 0;
    }
    else if (button >= mouseRange[0] && button <= mouseRange[1])
    {
        // device = static_cast<RE::BSInputDevice*>(inputManager->GetMouse());
        // button = button - mouseOffset;
    }
    else if (button >= 410 && button <= 473)
    {
        // device = static_cast<RE::BSInputDevice*>(inputManager->GetVRControllerLeft());
        // button = button - vrRightOffset;
    }
    else if (button >= 474)
    {
        // device = static_cast<RE::BSInputDevice*>(inputManager->GetVRControllerRight());
        // button = button - vrLeftOffset;
    }
    else
    {
        logger::error("Keycode not recognized: {}", button);
        return false;
    }

    // return device->IsPressed(button);

    logger::error("Keycode not recognized. This input device is not support. Please Contact a developer for more information. Button ID: {}", button);
    return false;  // Temporary line. This can be removed when commonLib updates to fix the issue causing the above line to crash

    // Don't know the offset for this
    //  bool isGamepadKeyDown = RE::BSInputDeviceManager::GetSingleton()->GetGamepad()->IsPressed((uint32_t)keyCode - gamepadOffset);
}

#pragma endregion Control whether a key is down for Keyboard, Mouse, Gamepad, or VR

#pragma region Menu Controls
// Open Journal menu (courtesy of shad0wshayd3)
void OpenJournal()
{
    // Spoof button input to open journal
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton())
    {
        auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "journal", 0, 1.0f, 0.0f);
        bsInputEventQueue->PushOntoInputQueue(kEvent);
    }

    /* // Also works
    REL::Relocation<void (*)(bool)> func{REL::VariantID(52428, 53327, 0x92F0F0)};
    return func(true); */
}

// Fade game screen. (courtesy of CustomSkills by Exit-9B https://github.com/Exit-9B/CustomSkills with help from doodlez and alandtse)
void FadeOutGame(bool a_fadingOut, bool a_blackFade, float a_fadeDuration, bool a_arg4, float a_secsBeforeFade)
{
    using func_t = decltype(&FadeOutGame);
    REL::Relocation<func_t> func{REL::VariantID(51909, 52847, 0x903080)};
    return func(a_fadingOut, a_blackFade, a_fadeDuration, a_arg4, a_secsBeforeFade);

    //using func_t = decltype(&FadeOutGame);
    //if (REL::Module::IsVR())
    //{
    //    // work in progress
    //    return;
    //}
    //else
    //{
    //    REL::Relocation<func_t> func{RELOCATION_ID(0, 52847)}; // Works for 1.6.640.0 ("AE")
    //    return func(a_fadingOut, a_blackFade, a_fadeDuration, a_arg4, a_secsBeforeFade);
    //}

    //----

    /*int fadeID = 0;*/
    //if (REL::Module::IsVR())
    //    fadeID = -1;
    //else
    //    fadeID = 52847; // Works for 1.6.640.0 ("AE")
    //REL::Relocation<func_t> func{RELOCATION_ID(0, fadeID)};
    //return func(a_fadingOut, a_blackFade, a_fadeDuration, a_arg4, a_secsBeforeFade);
}

// Open or close menu of interest
void MenuInteraction(MenuType type, MenuAction action)
{
    RE::BSFixedString menuName;
    RE::UI_MESSAGE_TYPE menuAction = RE::UI_MESSAGE_TYPE::kHide;
    if (action == MenuAction::Open) menuAction = RE::UI_MESSAGE_TYPE::kShow;
    switch (type)
    {  // Check if triggering menu is of interest
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
            if (playerXP >= playerLevelThreshold)
            {
                menuName = RE::StatsMenu::MENU_NAME;  // This will open the Skills menu, which will trigger the level up UI
                /// menuName = RE::LevelUpMenu::MENU_NAME; // This will directly call the level up UI
            }
            else
            {
                SendNotification("Not enough experience to level up");
                return;
            }
            break;
    }
    if (menuName == NULL) // Triggering menu is not of interest
    {
        logger::error("Error processing menu action - unexpected enum encountered");
        return;
    }
    else if (PlayerMorph() > 0 && (type == MenuType::Skills || type == MenuType::Console || type == MenuType::Favorites || type == MenuType::Journal) == false) // Check if player is werewolf or Vampire Lord AND an INVALID menu type was requested for actioning
    {
        SendNotification("Cannot do that while in alternate form");
        return;
    }
    else if (action == MenuAction::Open && openMenu == std::string(menuName.c_str())) // Check if action is to open a menu AND the target menu is already open
    {
        SendNotification("Menu already open");
        return;
    }
    else if (RE::UI::GetSingleton()->IsMenuOpen(RE::LevelUpMenu::MENU_NAME) == true) // Check if LevelUp menu is currently open
    {
        logger::debug("Can't open another menu while leveling up");
        ///SendNotification("Can't open another menu while leveling up");
        return;
    }
    std::thread([action, type, menuName, menuAction]() {
        if (action == MenuAction::Open && openMenu != "") // Check if action is to open a menu AND the open menu does not match the requested menu
        {
            auto* ui = RE::UI::GetSingleton();
            if (ui->IsMenuOpen(RE::MapMenu::MENU_NAME) == true && menuName == RE::JournalMenu::MENU_NAME) // Check if map is currently open and Journal was requested to open
            {
                /// SendNotification("Open Journal");
                OpenJournal();  // Open the JournalMenu (Quests tab)
                return;
            }
            else if (ui->IsMenuOpen(RE::JournalMenu::MENU_NAME) == true && ui->IsMenuOpen(RE::MapMenu::MENU_NAME) == true)  // Check if journal and map are both open
            {
                /// SendNotification("Auto Close Journal and Map");
                RE::UIMessageQueue::GetSingleton()->AddMessage(RE::JournalMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);  // Send message to close the Journal
                while (ui->IsMenuOpen(RE::JournalMenu::MENU_NAME) == true)
                    Sleep(250); // Brief pause to ensure open menu is closed before proceeding
                RE::UIMessageQueue::GetSingleton()->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kHide, nullptr);  // Send message to close the Map
                while (ui->IsMenuOpen(RE::MapMenu::MENU_NAME) == true)  // Brief pause to ensure open menu is closed before proceeding
                    Sleep(250);  // Brief pause to ensure open menu is closed before proceeding
            }
            else
            {
                ///SendNotification("Auto Close " + openMenu);
                ///logger::debug("Auto Close {}", openMenu);
                RE::UIMessageQueue::GetSingleton()->AddMessage(openMenu, RE::UI_MESSAGE_TYPE::kHide, nullptr);  // Send message to close the open menu
                while (ui->IsMenuOpen(openMenu) == true)  // Brief pause to ensure open menu is closed before proceeding
                    Sleep(250); // Brief pause to ensure open menu is closed before proceeding
            }
        }
        if (action == MenuAction::Open && menuName == RE::JournalMenu::MENU_NAME) // Check if Journal was requested to open
        {
            /// SendNotification("Open Journal");
            OpenJournal();  // Open the JournalMenu (Quests tab)
        }
        else if (action == MenuAction::Open && (menuName == RE::StatsMenu::MENU_NAME || menuName == RE::LevelUpMenu::MENU_NAME)) // Check if Skills menu or Level Up menu was requested to open
        {
            if (PlayerMorph() > 0) // Check if player is in Vampire Lord or Werewolf form
            {
                // Spoof button input to open werewolf or Vampire Lord skills menu
                if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton())
                {
                    RE::ButtonEvent *kEvent;
                    kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "Tween Menu", 0, 1.0f, 0.0f); // Spoof pressing "Tween Menu" button
                    bsInputEventQueue->PushOntoInputQueue(kEvent);
                }
            }
            else
            {
                FadeOutGame(true, true, 1.0f, true, 0.0f);  // Fade game so it appears as black background
                Sleep(100);                                 // Brief pause to ensure game (background) fading has time to initiate before proceeding
                RE::UIMessageQueue::GetSingleton()->AddMessage(menuName, menuAction, nullptr);  // Send message to open/close the target menu
            }
        }
        else
            RE::UIMessageQueue::GetSingleton()->AddMessage(menuName, menuAction, nullptr);  // Send message to open/close the target menu
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

    while (pos != std::string::npos)
    {
        std::string sub = "";

        sub = A.substr(pre_pos, (pos - pre_pos));

        if (pre_pos != pos)
        {
            sub = A.substr(pre_pos, (pos - pre_pos));
        }
        else
        {
            sub = A.substr(pre_pos, 1);
        }

        sub[0] = toupper(sub[0]);
        B += sub + A[pos];

        if (pos < (A.length() - 1))
        {
            pre_pos = (pos + 1);
        }
        else
        {
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
    for (auto playerMapMarker : *playerMapMarkers)
    {
        const auto refr = playerMapMarker.get().get();
        if (refr->IsDisabled() == false)
        {  // Check if map marker is NOT disabled
            const auto marker = refr ? refr->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
            if (marker && marker->mapData)
            {
                if (marker->mapData->flags.any(RE::MapMarkerData::Flag::kVisible) == true)
                {  // Check if map marker is visible
                    std::string markerName = marker->mapData->locationName.GetFullName();
                    std::transform(targetLocation.begin(), targetLocation.end(), targetLocation.begin(),
                                   [](unsigned char c) { return std::tolower(c); });  // sets type.ToLower()
                    std::transform(markerName.begin(), markerName.end(), markerName.begin(),
                                   [](unsigned char c) { return std::tolower(c); });  // sets type.ToLower()

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
    for (auto playerMapMarker : *playerMapMarkers)
    {
        const auto refr = playerMapMarker.get().get();
        if (refr->IsDisabled() == false)
        {  // Check if map marker is NOT disabled
            const auto marker = refr ? refr->extraList.GetByType<RE::ExtraMapMarker>() : nullptr;
            /// logger::debug("Game Location Name = {}", marker->mapData->locationName.GetFullName());
            if (marker && marker->mapData)
            {
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

    if (auto uiMessageQueue = RE::UIMessageQueue::GetSingleton())
    {
        RefHandleUIData* data = static_cast<RefHandleUIData*>(SKSE_CreateUIMessageData(RE::InterfaceStrings::GetSingleton()->refHandleUIData));
        data->refHandle = markerRef->GetHandle().native_handle();
        uiMessageQueue->AddMessage(RE::MapMenu::MENU_NAME, RE::UI_MESSAGE_TYPE::kUpdate, data);
    }
}

// Navigate to target location (marker) on world map
void NavigateToLocation(std::string targetLocation)
{
    RE::TESObjectREFR* mapMarkerRef = IsLocationKnown(targetLocation);
    if (mapMarkerRef != NULL)
    {
        std::string result = "Navigating to " + targetLocation;
        SendNotification("Location: " + Title_Case(targetLocation));
        logger::info("{}", result);
        FocusOnMapMarker(mapMarkerRef);
    }
    else
    {
        std::string result = targetLocation + " location is not known";
        SendNotification(result);
        logger::info("{}", result);
    }
}

// Navigate to player's position on world map
void NavigateToPlayer()
{
    // Spoof button input to navigate to player's position on world map
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton())
    {
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
void PlaceCustomMarker()
{
    // Spoof button input to navigate to player's position on world map
    if (auto bsInputEventQueue = RE::BSInputEventQueue::GetSingleton())
    {
        auto kEvent = RE::ButtonEvent::Create(RE::INPUT_DEVICE::kNone, "PlacePlayerMarker", 0, 1.0f, 0.0f);
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
// RE::PlayerCharacter::GetSingleton()->DrinkPotion
// RE::TESActor->SetSpeakingDone()
// LocalMapCamera(float a_zRotation);    //Maybe this can adjust the VR camera for object at the bottom, when you move to them. That way, it won't have
// mountains blocking them

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
//             std::string playerLocation = "Player = " + std::to_string(playerXPosition) + "," + std::to_string(playerYPosition) + "," +
//             std::to_string(playerZPosition); SendNotification(playerLocation.c_str()); logger::debug("{}", playerLocation);
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
//             std::string cameraAdjust = "Camera = " + std::to_string(cameraXOffset) + "," + std::to_string(cameraYOffset) + "," +
//             std::to_string(cameraZOffset); SendNotification(cameraAdjust.c_str()); logger::debug("{}", cameraAdjust);
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
// struct Mount
//{
//     static const int None = 0;
//     static const int Horse = RE::CameraState::kMount;
//     static const int Dragon = RE::CameraState::kDragon;
// };
#pragma endregion