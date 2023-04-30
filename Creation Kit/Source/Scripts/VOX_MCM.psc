Scriptname VOX_MCM extends SKI_ConfigBase

;Global Variables
GlobalVariable Property VOX_Enabled 				auto
GlobalVariable Property VOX_UpdateInterval 			auto
GlobalVariable Property VOX_PushToSpeakType 		auto
GlobalVariable Property VOX_PushToSpeakKeyCode 		auto
GlobalVariable Property VOX_AutoCastPowers 			auto
GlobalVariable Property VOX_AutoCastShouts 			auto
GlobalVariable Property VOX_ShoutKey 				auto
GlobalVariable Property VOX_ShowLog 				auto
GlobalVariable Property VOX_LongAutoCast 			auto
GlobalVariable Property VOX_Sensitivity 			auto
GlobalVariable Property VOX_CheckForUpdate 			auto
GlobalVariable Property VOX_KnownShoutWordsOnly 	auto


;Enable/Disable Mod
bool ToggleMod_Val = true
int ToggleMod_OID

;Push-To-Speak Menu
int pushToSpeakMenu_OID
int pushToSpeakKeyCode_OID
int pushToSpeakInput_OID
string[] pushToSpeakOptions
int PushToSpeakKeyCode_Default = -1

;Enable/Disable Auto-Cast Powers
bool ToggleAutoPowers_Val = true
int ToggleAutoPowers_OID

;Enable/Disable Auto-Cast Shouts
bool ToggleAutoShouts_Val = true
int ToggleAutoShouts_OID

;Auto-Cast Shout Key
int ShoutKey = 44
int SKey_OID

;Update Interval
float Default_UpdateInterval = 0.1
int Local_UpdateInterval

;Show Log
bool fVal = true
int fOID

;Voice Recognition Sensitivity	(in percentage where 0% is the least sensitive (full dictionary) and 100% is most sensitive (no dictionary)
int sensitivityDefaultValue = 10
int sensitivityValue = 10
int sensitivity_OID

int CheckUpdate

int longAutoCastOID

;Known Shout Words Only
int knownShoutWords_OID

event OnInit()
	parent.OnInit()

	;Fill in Push-To-Speak Options
	pushToSpeakOptions = new string[4]
	pushToSpeakOptions[0] = "Disabled"
	pushToSpeakOptions[1] = "Hold"
	pushToSpeakOptions[2] = "Toggle"
	pushToSpeakOptions[3] = "Vocal"
endEvent

Event OnPageReset(string page) 
	;Display:
	;
	;	"General"									|	"Other"
	;		(Toggle) Enable Voxima					|		(Slider) Update Interval
	;												|		(Slider) Sensitivity
	;		(Menu) Push-to-Speak (Non-VR)			|		(Toggle) Show Log
	;		(Keybind) Push-to-Speak Key (Non-VR)	|		("Button") Check for Update
	;												|		(Toggle) Auto-Cast Long Spells
	;	"Auto Cast"									|		(Toggle) Known Shout Words Only
	;		(Toggle ) Auto Cast Powers				|		
	;		(Toggle ) Auto Cast Shouts				|		
	;		(Keybind) Shout Key						|		
	;		(Input) Shout Key						|		
	;	
	;	
	;	
	;	
	;	
	
	
	SetCursorFillMode(TOP_TO_BOTTOM)

	AddHeaderOption("General")
	ToggleMod_OID = AddToggleOption("Enable Voxima", ToggleMod_Val)

	AddEmptyOption()

	;Push-to-Speak
	AddHeaderOption("Push-to-Speak (Non-VR)")
	pushToSpeakMenu_OID = AddMenuOption("Push To Speak Options", pushToSpeakOptions[VOX_PushToSpeakType.Value as int])
	
	if (VOX_PushToSpeakType.Value as int == 0)
		;Enable Additional Options
		pushToSpeakKeyCode_OID = AddKeyMapOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_DISABLED)
		pushToSpeakInput_OID = AddInputOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_DISABLED)
	else
		;Disable Additional Options (Grey them out)
		pushToSpeakKeyCode_OID = AddKeyMapOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_NONE)
		pushToSpeakInput_OID = AddInputOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_NONE)
	endif
	
	AddEmptyOption()
	
	;Auto-Cast Shouts and Powers
	AddHeaderOption("Auto Cast")
	ToggleAutoPowers_OID = AddToggleOption("Auto Cast Powers", ToggleAutoPowers_Val)
	ToggleAutoShouts_OID = AddToggleOption("Auto Cast Shouts", ToggleAutoShouts_Val)
	
	if (!ToggleAutoPowers_Val && !ToggleAutoShouts_Val)
		SKey_OID = AddKeyMapOption("Shout Key", VOX_ShoutKey.Value as int, OPTION_FLAG_DISABLED)
		ShoutKey = AddInputOption("Shout Key", VOX_ShoutKey.Value as int, OPTION_FLAG_DISABLED)
	else
		SKey_OID = AddKeyMapOption("Shout Key", VOX_ShoutKey.Value as int, OPTION_FLAG_NONE)
		ShoutKey = AddInputOption("Shout Key", VOX_ShoutKey.Value as int, OPTION_FLAG_NONE)
	endif

	SetCursorPosition(1) ; Move cursor to top right position

	;Update Interval
	AddHeaderOption("Other")
	Local_UpdateInterval = AddSliderOption("Update Interval", VOX_UpdateInterval.Value, "Every {2} Seconds")

	;Sensitivity Slider
	sensitivity_OID = AddSliderOption("Voice Recognition", sensitivityValue, "{2}% Sensitivity")	;The value is saved locally because otherwise, it may not save instantly
	
	;Show Log
	fOID = AddToggleOption("Show Log", fVal)
	
	;Check for Update
	CheckUpdate = AddToggleOption("Check for Update", VOX_CheckForUpdate.Value as bool)
	
	;Long Auto-Cast Spells
	longAutoCastOID = AddToggleOption("Auto-Cast Long Spells", VOX_LongAutoCast.Value as bool)
	
	;Known Shout Words Only
	KnownShoutWords_OID = AddToggleOption("Known Shout Words Only", VOX_KnownShoutWordsOnly.Value as bool)
	
EndEvent

;Event Triggered when a "menu" is opened
event OnOptionMenuOpen(int option)
	if (option == pushToSpeakMenu_OID)
		SetMenuDialogOptions(pushToSpeakOptions)
		SetMenuDialogStartIndex(VOX_PushToSpeakType.Value as int)
		SetMenuDialogDefaultIndex(0)
	endIf
endEvent

;Event Triggered when a menu item is chosen
event OnOptionMenuAccept(int option, int index)
	if (option == pushToSpeakMenu_OID)
		VOX_PushToSpeakType.SetValue(index)
		
		if (index == 0)
			SetOptionFlags(pushToSpeakKeyCode_OID, OPTION_FLAG_DISABLED)
			SetOptionFlags(pushToSpeakInput_OID, OPTION_FLAG_DISABLED)
		else
			SetOptionFlags(pushToSpeakKeyCode_OID, OPTION_FLAG_NONE)
			SetOptionFlags(pushToSpeakInput_OID, OPTION_FLAG_NONE)
		endif
		
		SetMenuOptionValue(pushToSpeakMenu_OID, pushToSpeakOptions[VOX_PushToSpeakType.Value as int])
	endIf
endEvent

Event OnOptionSelect(int option)

	if (option == ToggleMod_OID)
		ToggleMod_Val = !ToggleMod_Val
		VOX_Enabled.SetValue(ToggleMod_Val as Int)
		SetToggleOptionValue(ToggleMod_OID, ToggleMod_Val)
			
	;Auto-Cast Powers
	elseif (option == ToggleAutoPowers_OID)
		ToggleAutoPowers_Val = !ToggleAutoPowers_Val
		VOX_AutoCastPowers.SetValue(ToggleAutoPowers_Val as int)
		
		
		SetToggleOptionValue(ToggleAutoPowers_OID, ToggleAutoPowers_Val)
		
		if (!ToggleAutoPowers_Val && !ToggleAutoShouts_Val)
			SetOptionFlags(ShoutKey, OPTION_FLAG_DISABLED)
			SetOptionFlags(SKey_OID, OPTION_FLAG_DISABLED)
		else
			SetOptionFlags(ShoutKey, OPTION_FLAG_NONE)
			SetOptionFlags(SKey_OID, OPTION_FLAG_NONE)
		endif
		
	;Auto-Cast Shouts
	elseif (option == ToggleAutoShouts_OID)
		ToggleAutoShouts_Val = !ToggleAutoShouts_Val
		VOX_AutoCastShouts.SetValue(ToggleAutoShouts_Val as int)
		SetToggleOptionValue(ToggleAutoShouts_OID, ToggleAutoShouts_Val)
		
		if (!ToggleAutoPowers_Val && !ToggleAutoShouts_Val)
			SetOptionFlags(ShoutKey, OPTION_FLAG_DISABLED)
			SetOptionFlags(SKey_OID, OPTION_FLAG_DISABLED)
		else
			SetOptionFlags(ShoutKey, OPTION_FLAG_NONE)
			SetOptionFlags(SKey_OID, OPTION_FLAG_NONE)
		endif
		
	elseif (option == fOID)
		fVal = !fVal
		VOX_ShowLog.SetValue(fVal as int)
		SetToggleOptionValue(fOID, fVal)
		
	elseif (option == CheckUpdate)
		VOX_CheckForUpdate.SetValue(1)
		SetToggleOptionValue(CheckUpdate, VOX_CheckForUpdate.Value as bool)
		
	elseif (option == longAutoCastOID)
		VOX_LongAutoCast.SetValue((!(VOX_LongAutoCast.Value as bool)) as int)
		SetToggleOptionValue(longAutoCastOID, VOX_LongAutoCast.Value as bool)
		
	elseif (option == knownShoutWords_OID)
		VOX_KnownShoutWordsOnly.SetValue((!(VOX_KnownShoutWordsOnly.Value as bool)) as int)
		SetToggleOptionValue(knownShoutWords_OID, VOX_KnownShoutWordsOnly.Value as bool)
	endif
EndEvent

Event OnOptionKeyMapChange(int option, int keyCode, string conflictControl, string conflictName)
	if (option == pushToSpeakKeyCode_OID)
		VOX_PushToSpeakKeyCode.SetValue(keyCode)
		SetKeyMapOptionValue(pushToSpeakKeyCode_OID, VOX_PushToSpeakKeyCode.Value as int)
		SetInputOptionValue(pushToSpeakInput_OID, VOX_PushToSpeakKeyCode.Value as int)
		VOX_PushToSpeakKeycode.SetValue(keyCode)
	
	elseif (option == SKey_OID)
		VOX_ShoutKey.SetValue(keyCode)
		SetKeyMapOptionValue(SKey_OID, VOX_ShoutKey.Value as int)
		SetInputOptionValue(ShoutKey, VOX_ShoutKey.Value as int)
	
	endIf
EndEvent

;Sliders
Event OnOptionSliderOpen(int option)
	if (option == Local_UpdateInterval)
		SetSliderDialogStartValue(VOX_UpdateInterval.Value)
		SetSliderDialogDefaultValue(Default_UpdateInterval)
		SetSliderDialogRange(0.05, 5.0)
		SetSliderDialogInterval(0.05)
		
	elseif (option == sensitivity_OID)
		SetSliderDialogStartValue(sensitivityValue)
		SetSliderDialogDefaultValue(sensitivityDefaultValue)
		SetSliderDialogRange(1, 100)
		SetSliderDialogInterval(1)
	endif

EndEvent

Event OnOptionSliderAccept(int option, float value)
	if (option == Local_UpdateInterval)
		
		VOX_UpdateInterval.SetValue(value)
		
		if (value <= 1.0)
			SetSliderOptionValue(Local_UpdateInterval, VOX_UpdateInterval.Value, "Every {2} Second")
			
		else
			SetSliderOptionValue(Local_UpdateInterval, VOX_UpdateInterval.Value, "Every {2} Seconds")
			
		endif
	elseif (option == sensitivity_OID)
		
		sensitivityValue = value as int
		VOX_Sensitivity.SetValue(sensitivityValue)
		
		SetSliderOptionValue(sensitivity_OID, value as int, "{2}% Sensitivity")
		
	endif

EndEvent

Event OnOptionInputOpen(int option)
	;Fill input box with current value
	if (option == pushToSpeakInput_OID)
		SetInputDialogStartText(VOX_PushToSpeakKeyCode.Value as int)
		
	elseif (option == ShoutKey)
		SetInputDialogStartText(VOX_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, VOX_ShoutKey.Value as int)
	endif
	
EndEvent

Event OnOptionInputAccept(int option, string value)
	if (option == pushToSpeakInput_OID)
		VOX_PushToSpeakKeyCode.SetValue(value as int)
	
		SetInputOptionValue(pushToSpeakInput_OID, VOX_PushToSpeakKeyCode.Value as int)
		SetKeyMapOptionValue(pushToSpeakKeyCode_OID, VOX_PushToSpeakKeyCode.Value as int)
		
	elseif (option == ShoutKey)
		VOX_ShoutKey.SetValue(value as int)
	
		SetInputOptionValue(ShoutKey, VOX_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, VOX_ShoutKey.Value as int)
	endif
	
EndEvent

Event OnOptionDefault(int option)
	;Fill input box with current value
	if (option == pushToSpeakInput_OID || option == pushToSpeakKeyCode_OID)
		VOX_PushToSpeakKeyCode.SetValue(PushToSpeakKeyCode_Default)
	
		SetInputOptionValue(pushToSpeakInput_OID, VOX_PushToSpeakKeyCode.Value as int)
		SetKeyMapOptionValue(pushToSpeakKeyCode_OID, VOX_PushToSpeakKeyCode.Value as int)
		
	elseif (option == ShoutKey || option == SKey_OID)
		VOX_ShoutKey.SetValue(44)	;Input.GetMappedKey("Shout") gives the default key but this doesn't work on VR. value 44 is Z, which is the most common
	
		SetInputOptionValue(ShoutKey, VOX_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, VOX_ShoutKey.Value as int)
	
	elseif (option == sensitivity_OID)
		sensitivityValue = sensitivityDefaultValue
		VOX_Sensitivity.SetValue(sensitivityValue)
		
		SetSliderOptionValue(sensitivity_OID, sensitivityValue,"{2}% Sensitivity")
	
	elseif (option == longAutoCastOID)
		VOX_LongAutoCast.SetValue(0)
		SetToggleOptionValue(longAutoCastOID, VOX_LongAutoCast.Value as bool)
	
	elseif (option == knownShoutWords_OID)
		VOX_KnownShoutWordsOnly.SetValue(1)
		SetToggleOptionValue(knownShoutWords_OID, VOX_KnownShoutWordsOnly.Value as bool)
		
	endif

EndEvent

Event OnOptionHighlight(int option)
	if (option == ToggleMod_OID)
		SetInfoText("Enable/Disable Voice Recognition")
		
	elseif (option == pushToSpeakKeyCode_OID)
		SetInfoText("The key you press to enable Voice Recognition")
	
	elseif (option == pushToSpeakMenu_OID)
		SetInfoText("Select Push-To-Speak Type.\nVocal: Say designated start listening and stop listening commands to enable/disable recognition. Default is \"Stat/Stop Listening\"\nHold: Hold the designated key to enable recognition\nToggle: Press the designated key to toggle recognition")
		;SetInfoText("Say designated start listening and stop listening commands to enable/disable recognition. Default is \"Stat/Stop Listening\"")
		
	elseif (option == ToggleAutoPowers_OID)
		SetInfoText("Enable/Disable auto-casting powers after they are equipped by voice recognition")
		
	elseif (option == ToggleAutoShouts_OID)
		SetInfoText("Enable/Disable auto-casting shouts after they are equipped by voice recognition")
		
	elseif (option == SKey_OID)
		SetInfoText("Default: Set the key to the game's most common shout keybind, Z")
		
	elseif (option == Local_UpdateInterval)
		SetInfoText("The time in seconds the game will wait before checking for an update from the voice recognition program.\nHigher values lead to longer wait times after each command")
		
	elseif (option == fOID)
		SetInfoText("Enable/Disable the log in the top left corner after a command is recognized")
		
	elseif (option == longAutoCastOID)
		SetInfoText("This enables auto-casting of spells with long cast times, such as \"Frost Thrall\" and \"Mass Paralysis\".\nThis is disabled by default for balancing purposes")
		
	elseif (option == knownShoutWords_OID)
		SetInfoText("Disabling this allows you do say higher shout leves to cast the most powerful version you have\nExample: If you own \"Fus Ro\", but not \"Dah\", you can say \"Fus Ro Dah\" and \"Fus Ro\" will be cast")
		
	elseif (option == sensitivity_OID)
		SetInfoText("Adjusts how sensitive the Voice Recognition is. (1-100 from least to most sensitive)\nThis can help if commands aren't recognized easily or if breathing/talking activates commands")
	
	endIf
	
EndEvent





