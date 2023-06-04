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
	

## Acknowledgements
* Ryan McKenzie, powerof3, and CharmedBaryon for CommonLibSSE (and NG), and all the folks who contributed to these
* Joel Christner for developing WatsonWebSocket, and Younes Cheikh for developing ObservableConcurrentQueue
* Boost.org for developing the boost random library
* MrowPurr for her CommonLib-NG templates, excellent YouTube tutorials, headstart on websocket integration for C++, and other helpful input
* MrowPurr's Discord community for various help and encouragement
* CharmedBaryon, Fenix31415, Nightfallstorm, Noah Buddie, Nukem, Qudix, shad0wshayd3, and others within the SkyrimSE RE Discord community for a variety of help and advice
* All the awesome folks who make their SKSE plugin source public
* Bethesda for giving us Skyrim and its Creation Kit
