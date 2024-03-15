# Voxima - Skyrim Voice Control

This is the readme

## Development Resources
check and verify language & speech engine ==> https://learn.microsoft.com/en-us/windows/apps/design/input/specify-the-speech-recognizer-language

## Features
### Equipping
  **Spells** - Say the name of the spell and which hand(s) it will go into (`Flames Left`)
  
  **Powers** - Say the name of the power. If Auto-Cast Powers is disabled, it will equip the power (`Adrenaline Rush`)
  
  **Shouts** - Say the dovahzul words of the shout. If Auto-Cast Shouts is disabled, it will equip the shout (`Fus Ro Dah`)

### Casting
  **Spells** - Say "cast", the name of the spell, and which hand(s) it will cast from (`Cast Flames Left`)
  
  **Powers** - Say the name of the power. If Auto-Cast Powers is disabled, you need to add "cast" to the beginning (`Cast Adrenaline Rush`)
  
  **Shouts** - Say the dovahzul words of the shout. If Auto-Cast Shouts is enable, it will cast whichever words you spoke (`Fus Ro Dah`)
  
  For Powers and Shouts, anything you previously had in your voice slot will remain, so if you have Adrenaline Rush equipped, then auto-cast Unrelenting Force, pressing the shout button will activate Adrenaline Rush, not Unrelenting Force
  
### Map Navigation
  **Move to a specified location** - By saying "go to *location*", the map will move to it. this works for any known locations (`Go to Riften`)
  
  **Place custom Marker** - You can hover your mouse over a place and say "Place Custom Marker" place one (`Place Custom Marker`)
  
  **Move to Player** - You can say "move to player" to navigate back to your player's location (`Move to Player`)
  
### Menu Controls
  **Open Menus** - By saying "open" and the name of the menu, you can open it (`Open Spellbook`)
  
  **Close Menus** - By saying "close" and the name of the menu, you can close it (`Close Spellbook`)
  
  **Quick Save** - Creates a quick save by saying "quick save"
  
  **Quick Load** - Reloads the previous save by saying "quick load"
 
### Mount Controls
  **Horse Controls** - The available commands are "Forward", "Stop", "Jump", "Go Left", "Turn Left", "Go Right", "Turn Right", "Turn Around", "Sprint", "Stop Sprinting", "Run", "Stop Running", "Walk", "Stop Walking", "Rear Up", "Go Backwards", "Faster", "Slower", "Dismount"
  
  **Dragon Controls** - The available commands are "Land", "Next Target", "Previous Target", "Attack", "Lock Target"
  
### Progressions
  **Progressions** - This mod features a unique "progressions" system that allows a single command to equip a different item depening on what your character knows. One examples is "Raise Dead". This command will equip or cast the most powerful spell you own that can raise the dead (excluding Dead Thrall)
  
### Console Commands
  This mod allows you to create strings of console commands that have one or multiple commands that will trigger them.
  
  In addition to console commands, you can add a `wait` command to the strings to wait a certain amount of time before executing the next command.
  You can also add `press #`, `hold #`, and `release #` commands to manipulate keys on a keyboard.
  
  **Example 1:** "Summon the bandit army = player.placeatme 1068FE * 10 + wait 1s + player.placeatme 1E60D * 5" - This will summon 10 regular bandits, wait 1 second, then summon 5 stronger bandits whenver you say the command "Summon the bandit army".
  
  **Example 2:** "(I'm ready to die)(Make it hurt) = player.placeatme F80FA + player.placeatme 4247E + press Space 10s" - This will summon a dragon and a strong drauger, the hold the space bar for 10 seconds every time you say either "I'm ready to die" or "Make it hurt".
  
### Enable/Disable Recognition
  **Disabled** - Recognition is always enabled
  
  **Hold** - Recognition is enabled while the specified key is held
  
  **Press** - Recognition is enabled when the specified key is pressed, then disabled when it is pressed again
  
  **Vocal** - Two new commands are enabled: "Enable Voice Recognition" and "Disable Voice Recognition". These do what you'd expect them to
  
### Custom Keybinds
  If you'd prefer certain keybind that are only active when the player is in a certain state, you can do that too!
  The supported states are: Regular Player, Dragon-Riding, Horse-Riding
  
### Misc
  **Werewolf Roar** - When in beast form, there is a special command, "roar", to trigger your werewolf shout
  
  **Unequip Hands** - By saying "Unequip Hands", any items/magic will be removed from your hands
  
  **Unequip Voice** - Bt saying "Unequip Voice", any power or shout in your voice slot will be unequipped
  
  ##
  Most commands shown here have many variations to them to help them feel more natural. To see all variations, look at the mods files

## Setup From the Beginning WIP
This is the overall process we followed to get everything ready so we could develop and subsequently use Voxima. Feel free to make your own adjustments based on your preferences.

1. WIP Install Skyrim. Voxima is compatible with:
   - Flatrim version XYZ
   - SkyrimVR version XYZ
   - Install outside of Program Files?
2. WIP Install Skyrim Creation Kit:
   - Where is it found?
   - Any version dependency?
   - Install outside of Program Files?
3. Download [Visual Studio](https://visualstudio.microsoft.com/) (the free Community edition is fine)
4. Clone [vcpkg](https://github.com/microsoft/vcpkg) (we used Github Desktop to facilitate this)
   - Go into the `vcpkg` folder and run `bootstrap-vcpkg.bat`. This should create a `vcpkg.exe` file.
   - Also make note of (i.e. copy) the path to your `vcpkg` folder, since we'll need it in the next step. 
5. Edit your system or user Environment Variables to add a new one
   - Press the `Windows` keyboard key and type in `environment`, which should execute a search.
   - Click the match that reads something like `Edit environment variables for your account`.
   - Click `New` and this window should appear:
     ![image](https://github.com/zbigdogz/Voxima-SkyrimVoiceControl/assets/31357974/f47c5716-a9c5-44ee-b808-c66087677ea1)
   - Enter the following in the fields:
     - Variable name: `VCPKG_ROOT`
     - Variable value: `C:\path\to\wherever\your\vcpkg\folder\is`
       - As an example, in my case my `Variable value` was `D:\Documents\Source\Github\vcpkg` (again, your `Variable value` must reflect where your `vcpkg` folder is located)
   - Press `OK` to confirm on this window and press `OK` on the `Environment Variables` window that's still open
6. Install and configure a mod organizer for managing your Skyrim mods. See YouTube tutorials by GamerPoets for help with installation and configuration. Compatible options include:
   - Mod Organizer 2
   - Vortex Mod Manager
7. Configure this project to output your plugin files into your "mods" folder (for Mod Organizer 2 or Vortex):
   - Create a `SKYRIM_MODS_FOLDER` environment variable to the path of your mods folder (similar process as step 5):
      - e.g. G:\ModOrganizer\Skyrim Special Edition\mods
      - e.g. G:\Vortex\skyrimse\mods
8. Clone the `Voxima-SkyrimVoiceControl` repository (again, we used Github Desktop here)
9. Open the `Voxima-SkyrimVoiceControl.sln` file (inside `Voxima-SkyrimVoiceControl\Voice Recognition App\`) with Visual Studio
10. Build the `Voxima-SkyrimVoiceControl` solution. Generated files should automatically be placed wherever `SKYRIM_MODS_FOLDER` is pointing.
11. Open the `Voxima-SkyrimVoiceControl` plugin in Visual Studio
    - Navigate to `\Voxima-SkyrimVoiceControl\Plugin`, right-click on white space within the folder, and select `Open with Visual Studio`.
    - Note that the first time you do this it might take a while for Visual Studio to gather all of the plugin dependencies.
    - Wait until the `Output` log reads `CMake generation finished` before going to the next step.
12. Build the `Voxima-SkyrimVoiceControl` plugin
    - Click on the `Build` menu option and select `Build All`. Generated files should automatically be placed wherever `SKYRIM_MODS_FOLDER` is pointing.
    - If you encounter the below (syntax) error...
      
      ![image](https://github.com/zbigdogz/Voxima-SkyrimVoiceControl/assets/31357974/f9c9c942-22a0-442b-ada7-8d89d8f3eb8b)
      
      ...then simply revise line 11 to be `.MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }`
13. Make sure `Voxima - Skyrim Voice Control` is enabled in your mod manager
14. Launch Skyrim through your mod manager
15. Profit

WIP Keep working on this section
https://github.com/SkyrimScripting/SKSE_Template_HelloWorld
https://github.com/SkyrimScripting/SKSE_Templates

https://www.youtube.com/playlist?list=PLektTyeQhBZeDIRp2g15SsK1GX2Ig8YVW

## Acknowledgements
* Ryan McKenzie, powerof3, and CharmedBaryon for CommonLibSSE (and NG), and all the folks who contributed to these
* Joel Christner for developing [WatsonWebSocket](https://github.com/jchristn/WatsonWebsocket), and Younes Cheikh for developing [ObservableConcurrentQueue](https://github.com/YounesCheikh/ObservableConcurrentQueue)
* Boost.org for developing the boost random library
* [MrowPurr](https://github.com/SkyrimScripting) (also known for SkyrimScripting) for her CommonLib-NG templates, excellent YouTube tutorials, headstart on websocket integration for C++, and other helpful input
* MrowPurr's Discord community for various help and encouragement
* CharmedBaryon, Fenix31415, Nightfallstorm, Noah Buddie, Nukem, Qudix, shad0wshayd3, and others within the SkyrimSE RE Discord community for a variety of help and advice
* All the awesome folks who make their SKSE plugin source public
* Bethesda for giving us Skyrim and its Creation Kit
