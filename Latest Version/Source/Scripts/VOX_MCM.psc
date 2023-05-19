Scriptname VOX_MCM extends SKI_ConfigBase

;Global Variables
GlobalVariable Property VOX_Enabled 				auto
GlobalVariable Property VOX_UpdateInterval 			auto
GlobalVariable Property VOX_PushToSpeakType 		auto
GlobalVariable Property VOX_PushToSpeakKeyCode 		auto
GlobalVariable Property VOX_AutoCastPowers 			auto
GlobalVariable Property VOX_AutoCastShouts 			auto
GlobalVariable Property VOX_ShowLog 				auto
GlobalVariable Property VOX_LongAutoCast 			auto
GlobalVariable Property VOX_Sensitivity 			auto
GlobalVariable Property VOX_CheckForUpdate 			auto
GlobalVariable Property VOX_KnownShoutWordsOnly 	auto
GlobalVariable Property VOX_QuickLoadType		 	auto


;Enable/Disable Mod
	int ToggleMod_OID

;Push-To-Speak Menu
	int pushToSpeakMenu_OID
	int pushToSpeakKeyCode_OID
	int pushToSpeakInput_OID
	string[] pushToSpeakOptions
	int PushToSpeakKeyCode_Default = -1

;Enable/Disable Auto-Cast Powers
	int ToggleAutoPowers_OID

;Enable/Disable Auto-Cast Shouts
	int ToggleAutoShouts_OID

;Show Log
	int ShowLog_OID

;Voice Recognition Sensitivity	(in percentage where 0% is the least sensitive (full dictionary) and 100% is most sensitive (no dictionary)
	int sensitivityDefaultValue = 10
	int sensitivity_OID

;Force the mod to look for changes
int CheckUpdate

;Allow Auto-Cast for Spells with a long cast time
int longAutoCastOID

;Known Shout Words Only
	int knownShoutWords_OID
	int knownShoutWords_Default = 1

;Quick Load Settings
	int QuickLoadType_OID
	string[] quickLoadOptions
	int QuickLoadType_Default = 2
	

;Update Interval	(This is not currently used. It may be removed in the future, if it continues to not be used)
	;float Default_UpdateInterval = 0.1
	;int Local_UpdateInterval


event OnInit()
	parent.OnInit()

	;Fill in Push-To-Speak Options
	pushToSpeakOptions = new string[4]
	pushToSpeakOptions[0] = "Disabled"
	pushToSpeakOptions[1] = "Hold"
	pushToSpeakOptions[2] = "Press"
	pushToSpeakOptions[3] = "Vocal"

	;Fill in Quick Load Options
	quickLoadOptions = new string[3]
	quickLoadOptions[0] = "Disabled"
	quickLoadOptions[1] = "Show Confirmation"
	quickLoadOptions[2] = "No Confirmation"
endEvent

Event OnPageReset(string page) 
	;Display:
	;
	;	"General"									|	"Other"
	;		(Toggle) Enable Voxima					|		(Slider) Update Interval
	;												|		(Slider) Sensitivity
	;		(Menu) Push-to-Speak					|		(Toggle) Show Log
	;		(Keybind) Push-to-Speak Key				|		("Button") Check for Update
	;		(Input) Push-to-Speak Input				|		(Toggle) Auto-Cast Long Spells
	;												|		(Toggle) Known Shout Words Only
	;	"Auto Cast"									|		(Menu) Quick Load Options
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
	ToggleMod_OID = AddToggleOption("Enable Voxima", VOX_Enabled.Value as int)

	AddEmptyOption()

	;Push-to-Speak
	AddHeaderOption("Push-to-Speak")
	pushToSpeakMenu_OID = AddMenuOption("Push To Speak Options", pushToSpeakOptions[VOX_PushToSpeakType.Value as int])
	
	if (VOX_PushToSpeakType.Value as int == 0 || VOX_PushToSpeakType.Value as int == 3)
		;Disable Additional Options (Grey them out)
		pushToSpeakKeyCode_OID = AddKeyMapOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_DISABLED)
		pushToSpeakInput_OID = AddInputOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_DISABLED)
	else
		;Eanble Additional Options
		pushToSpeakKeyCode_OID = AddKeyMapOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_NONE)
		pushToSpeakInput_OID = AddInputOption("Key", VOX_PushToSpeakKeyCode.Value as int, OPTION_FLAG_NONE)
	endif
	
	AddEmptyOption()
	
	;Auto-Cast Shouts and Powers
	AddHeaderOption("Auto Cast")
	ToggleAutoPowers_OID = AddToggleOption("Auto Cast Powers", VOX_AutoCastPowers.Value as int)
	ToggleAutoShouts_OID = AddToggleOption("Auto Cast Shouts", VOX_AutoCastShouts.Value as int)

	SetCursorPosition(1) ; Move cursor to top right position

	;Update Interval
	AddHeaderOption("Other")
	;Local_UpdateInterval = AddSliderOption("Update Interval", VOX_UpdateInterval.Value, "Every {2} Seconds")

	;Sensitivity Slider
	sensitivity_OID = AddSliderOption("Voice Recognition", VOX_Sensitivity.Value as int, "{2}% Sensitivity")	;The value is saved locally because otherwise, it may not save instantly
	
	;Show Log
	ShowLog_OID = AddToggleOption("Show Log", VOX_ShowLog.Value as int)
	
	;Check for Update
	CheckUpdate = AddToggleOption("Check for Update", VOX_CheckForUpdate.Value as bool)
	
	;Long Auto-Cast Spells
	longAutoCastOID = AddToggleOption("Auto-Cast Long Spells", VOX_LongAutoCast.Value as bool)
	
	;Known Shout Words Only
	KnownShoutWords_OID = AddToggleOption("Known Shout Words Only", VOX_KnownShoutWordsOnly.Value as bool)
	
	;Quick Load Options
	QuickLoadType_OID = AddMenuOption("Quick Load Options", quickLoadOptions[VOX_QuickLoadType.Value as int])
	
EndEvent

;Event Triggered when a "menu" is opened
event OnOptionMenuOpen(int option)
	if (option == pushToSpeakMenu_OID)
		SetMenuDialogOptions(pushToSpeakOptions)
		SetMenuDialogStartIndex(VOX_PushToSpeakType.Value as int)
		SetMenuDialogDefaultIndex(0)
		
	elseif (option == QuickLoadType_OID)
		SetMenuDialogOptions(quickLoadOptions)
		SetMenuDialogStartIndex(VOX_QuickLoadType.Value as int)
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
		
		SetMenuOptionValue(pushToSpeakMenu_OID, pushToSpeakOptions[index])
		
		elseif (option == QuickLoadType_OID)
		VOX_QuickLoadType.SetValue(index)
		SetMenuOptionValue(QuickLoadType_OID, quickLoadOptions[index])
		
	endIf
endEvent

Event OnOptionSelect(int option)

	if (option == ToggleMod_OID)
		VOX_Enabled.SetValue((!(VOX_Enabled.Value as bool)) as int)
		SetToggleOptionValue(ToggleMod_OID, VOX_Enabled.Value as int)
			
	;Auto-Cast Powers
	elseif (option == ToggleAutoPowers_OID)
		VOX_AutoCastPowers.SetValue((!(VOX_AutoCastPowers.Value as bool)) as int)
		
		
		SetToggleOptionValue(ToggleAutoPowers_OID, VOX_AutoCastPowers.Value as int)
		
	;Auto-Cast Shouts
	elseif (option == ToggleAutoShouts_OID)
		VOX_AutoCastShouts.SetValue((!(VOX_AutoCastShouts.Value as bool)) as int)
		SetToggleOptionValue(ToggleAutoShouts_OID, VOX_AutoCastShouts.Value as int)
		
	elseif (option == ShowLog_OID)
		VOX_ShowLog.SetValue((!(VOX_ShowLog.Value as bool)) as int)
		SetToggleOptionValue(ShowLog_OID, VOX_ShowLog.Value as int)
		
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
	endif
EndEvent

;Sliders
Event OnOptionSliderOpen(int option)
	;if (option == Local_UpdateInterval)
	;	SetSliderDialogStartValue(VOX_UpdateInterval.Value)
	;	SetSliderDialogDefaultValue(Default_UpdateInterval)
	;	SetSliderDialogRange(0.05, 5.0)
	;	SetSliderDialogInterval(0.05)
	;	
	
	if (option == sensitivity_OID)
		SetSliderDialogStartValue(VOX_Sensitivity.Value as int)
		SetSliderDialogDefaultValue(sensitivityDefaultValue)
		SetSliderDialogRange(1, 100)
		SetSliderDialogInterval(1)
	endif

EndEvent

Event OnOptionSliderAccept(int option, float value)
	;if (option == Local_UpdateInterval)
	;	
	;	VOX_UpdateInterval.SetValue(value)
	;	
	;	if (value <= 1.0)
	;		SetSliderOptionValue(Local_UpdateInterval, VOX_UpdateInterval.Value, "Every {2} Second")
	;		
	;	else
	;		SetSliderOptionValue(Local_UpdateInterval, VOX_UpdateInterval.Value, "Every {2} Seconds")
	;		
	;	endif
	
	if (option == sensitivity_OID)
		
		VOX_Sensitivity.SetValue(value as int)
		
		SetSliderOptionValue(sensitivity_OID, value as int, "{2}% Sensitivity")
		
	endif

EndEvent

Event OnOptionInputOpen(int option)
	;Fill input box with current value
	if (option == pushToSpeakInput_OID)
		SetInputDialogStartText(VOX_PushToSpeakKeyCode.Value as int)
	endif
	
EndEvent

Event OnOptionInputAccept(int option, string value)
	if (option == pushToSpeakInput_OID)
		VOX_PushToSpeakKeyCode.SetValue(value as int)
	
		SetInputOptionValue(pushToSpeakInput_OID, VOX_PushToSpeakKeyCode.Value as int)
		SetKeyMapOptionValue(pushToSpeakKeyCode_OID, VOX_PushToSpeakKeyCode.Value as int)
	endif
	
EndEvent

Event OnOptionDefault(int option)
	;Fill input box with current value
	if (option == pushToSpeakInput_OID || option == pushToSpeakKeyCode_OID)
		VOX_PushToSpeakKeyCode.SetValue(PushToSpeakKeyCode_Default)
	
		SetInputOptionValue(pushToSpeakInput_OID, VOX_PushToSpeakKeyCode.Value as int)
		SetKeyMapOptionValue(pushToSpeakKeyCode_OID, VOX_PushToSpeakKeyCode.Value as int)
	
	elseif (option == sensitivity_OID)
		VOX_Sensitivity.SetValue(sensitivityDefaultValue)
		
		SetSliderOptionValue(sensitivity_OID, VOX_Sensitivity.Value as int,"{2}% Sensitivity")
	
	elseif (option == longAutoCastOID)
		VOX_LongAutoCast.SetValue(0)
		SetToggleOptionValue(longAutoCastOID, VOX_LongAutoCast.Value as bool)
	
	elseif (option == knownShoutWords_OID)
		VOX_KnownShoutWordsOnly.SetValue(knownShoutWords_Default)
		SetToggleOptionValue(knownShoutWords_OID, VOX_KnownShoutWordsOnly.Value as bool)
	
	elseif (option == QuickLoadType_OID)
		VOX_QuickLoadType.SetValue(QuickLoadType_Default)
		SetToggleOptionValue(QuickLoadType_OID, VOX_QuickLoadType.Value as int)
		
	endif

EndEvent

Event OnOptionHighlight(int option)
	if (option == ToggleMod_OID)
		SetInfoText("Enable/Disable Voice Recognition")
		
	elseif (option == pushToSpeakKeyCode_OID)
		SetInfoText("The key you press to enable Voice Recognition (VR buttons do not display properly, but they will still work)")
	
	elseif (option == pushToSpeakMenu_OID)
		SetInfoText("Select Push-To-Speak Type.\nHold: Hold the designated key to enable recognition\nPress: Press the designated key to toggle recognition\nVocal: Say designated start listening and stop listening commands to enable/disable recognition. Default is \"Stat/Stop Listening\"")
		;SetInfoText("Say designated start listening and stop listening commands to enable/disable recognition. Default is \"Stat/Stop Listening\"")
		
	elseif (option == ToggleAutoPowers_OID)
		SetInfoText("Enable/Disable auto-casting powers after they are equipped by voice recognition")
		
	elseif (option == ToggleAutoShouts_OID)
		SetInfoText("Enable/Disable auto-casting shouts after they are equipped by voice recognition")
		
	;elseif (option == Local_UpdateInterval)
	;	SetInfoText("The time in seconds the game will wait before checking for an update from the voice recognition program.\nHigher values lead to longer wait times after each command")
		
	elseif (option == ShowLog_OID)
		SetInfoText("Enable/Disable the log in the top left corner after a command is recognized")
		
	elseif (option == longAutoCastOID)
		SetInfoText("This enables auto-casting of spells with long cast times, such as \"Frost Thrall\" and \"Mass Paralysis\".\nThis is disabled by default for balancing purposes")
		
	elseif (option == QuickLoadType_OID)
		SetInfoText("Decides how the game handles the \"Quick Load\" Voice command\n\"Disabled\" - This command will not work\n\"Show Confirmation\" - A confirmation window will appear\n\"No Confirmation\" - This command will load the previous save without any confirmation")
		
	elseif (option == knownShoutWords_OID)
		SetInfoText("Disabling this allows you do say higher shout leves to cast the most powerful version you have\nExample: If you own \"Fus Ro\", but not \"Dah\", you can say \"Fus Ro Dah\" and \"Fus Ro\" will be cast")
		
	elseif (option == sensitivity_OID)
		SetInfoText("Adjusts how sensitive the Voice Recognition is. (1-100 from least to most sensitive)\nThis can help if commands aren't recognized easily or if breathing/talking activates commands")
	
	endIf
	
EndEvent





