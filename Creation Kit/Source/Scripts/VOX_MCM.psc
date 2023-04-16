Scriptname VOX_MCM extends SKI_ConfigBase

;Global Variables
GlobalVariable Property VOX_Enabled auto
GlobalVariable Property VOX_UpdateInterval auto
GlobalVariable Property VOX_PushToSpeak auto
GlobalVariable Property VOX_VocalPushToSpeak auto
GlobalVariable Property VOX_AutoCastPowers auto
GlobalVariable Property VOX_AutoCastShouts auto
GlobalVariable Property VOX_ShoutKey auto
GlobalVariable Property VOX_ShowLog auto
GlobalVariable Property VOX_LongAutoCast auto
GlobalVariable Property VOX_Sensitivity auto
GlobalVariable Property VOX_CheckForUpdate auto


;Enable/Disable Recognition
bool aVal = true
int aOID

;Enable/Disable Push-To-Speak (PTS)
bool bVal = false
int bOID

;Keypress Push-To-Speak Key
int cVal = -1
int cOID

;PTS Value (Manual)
int PTS_KeyCode

;Enable/Disable Vocal Push-To-Speak (PTS)
bool VPTS_Val = false
int VPTS_OID

;Enable/Disable Auto-Cast Powers
bool dVal = true
int dOID

;Enable/Disable Auto-Cast Shouts
bool eVal = true
int eOID

;Auto-Cast Shout Key
int ShoutKey = 44
int SKey_OID


float Default_UpdateInterval = 0.1
int Local_UpdateInterval

bool fVal = true
int fOID

;Voice Recognition Sensitivity	(in percentage where 0% is the least sensitive (full dictionary) and 100% is most sensitive (no dictionary)
int sensitivityDefaultValue = 10
int sensitivityValue = 10
int sensitivity_OID

int CheckUpdate

int longAutoCastOID


Event OnPageReset(string page) 
	;Display:
	;
	;	"General"									|	"Other"
	;		(Toggle) Voice Recognition				|		(Slider) Update Interval
	;												|		(Slider) Sensitivity
	;		(Toggle) Push-to-Speak (Non-VR)			|		(Toggle) Show Log
	;		(Keybind) Push-to-Speak Key (Non-VR)	|		("Button") Check for Update
	;		(Toggle) Vocal Speech Toggle			|		
	;												|		
	;	"Auto Cast"									|		
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
	aOID = AddToggleOption("Voice Recognition", aVal)

	AddEmptyOption()

	;Push-to-Speak
	AddHeaderOption("Push-to-Speak (Non-VR)")
	bOID = AddToggleOption("Enabled", bVal)
	if (!bVal)
		cOID = AddKeyMapOption("Key", cVal, OPTION_FLAG_DISABLED)
		PTS_KeyCode = AddInputOption("Key", cVal, OPTION_FLAG_DISABLED)
	else
		cOID = AddKeyMapOption("Key", cVal, OPTION_FLAG_NONE)
		PTS_KeyCode = AddInputOption("Key", cVal, OPTION_FLAG_NONE)
	endif
	
	AddHeaderOption("Vocal Speech Toggle")
	VPTS_OID = AddToggleOption("Enabled", VPTS_Val)
	
	AddEmptyOption()
	
	AddHeaderOption("Auto Cast")
	dOID = AddToggleOption("Auto Cast Powers", dVal)
	eOID = AddToggleOption("Auto Cast Shouts", eVal)
	
	if (!dVal && !eVal)
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

	sensitivity_OID = AddSliderOption("Voice Recognition", sensitivityValue, "{2}% Sensitivity")	;The value is saved locally because otherwise, it may not save instantly
	
	;Show Log
	fOID = AddToggleOption("Show Log", fVal)
	
	;Check for Update
	CheckUpdate = AddToggleOption("Check for Update", VOX_CheckForUpdate.Value as bool)
	
	;Long Auto-Cast Spells
	longAutoCastOID = AddToggleOption("Auto-Cast Long Spells", VOX_LongAutoCast.Value as bool)
	
EndEvent

Event OnOptionSelect(int option)

	if (option == aOID)
		aVal = !aVal
		VOX_Enabled.SetValue(aVal as Int)
		SetToggleOptionValue(aOID, aVal)
		
	;Push-To-Speak Toggle
	elseif (option == bOID)
		bVal = !bVal 
		SetToggleOptionValue(bOID, bVal)
		
		if (!bVal)
			VOX_PushToSpeak.SetValue(-1)
			SetOptionFlags(cOID, OPTION_FLAG_DISABLED)
			SetOptionFlags(PTS_KeyCode, OPTION_FLAG_DISABLED)
		else
			VOX_PushToSpeak.SetValue(cVal)
			SetOptionFlags(cOID, OPTION_FLAG_NONE)
			SetOptionFlags(PTS_KeyCode, OPTION_FLAG_NONE)
		endif
	
	;Vocal Push-to-Speak
	elseif (option == VPTS_OID)
		VPTS_Val = !VPTS_Val
		VOX_CheckForUpdate.SetValue(1)
		VOX_VocalPushToSpeak.SetValue(VPTS_Val as int)
		SetToggleOptionValue(VPTS_OID, VPTS_Val)
			
	;Auto-Cast Powers
	elseif (option == dOID)
		dVal = !dVal
		VOX_AutoCastPowers.SetValue(dVal as int)
		
		
		SetToggleOptionValue(dOID, dVal)
		
		if (!dVal && !eVal)
			SetOptionFlags(ShoutKey, OPTION_FLAG_DISABLED)
			SetOptionFlags(SKey_OID, OPTION_FLAG_DISABLED)
		else
			SetOptionFlags(ShoutKey, OPTION_FLAG_NONE)
			SetOptionFlags(SKey_OID, OPTION_FLAG_NONE)
		endif
		
	;Auto-Cast Shouts
	elseif (option == eOID)
		eVal = !eVal
		VOX_AutoCastShouts.SetValue(eVal as int)
		SetToggleOptionValue(eOID, eVal)
		
		if (!dVal && !eVal)
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
	endif
EndEvent

Event OnOptionKeyMapChange(int option, int keyCode, string conflictControl, string conflictName)
	if (option == cOID)
		cVal = keyCode
		SetKeyMapOptionValue(cOID, cVal)
		SetInputOptionValue(PTS_KeyCode, cVal)
		VOX_PushToSpeak.SetValue(keyCode)
	
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
	if (option == PTS_KeyCode)
		SetInputDialogStartText(cVal)
		
	elseif (option == ShoutKey)
		SetInputDialogStartText(VOX_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, VOX_ShoutKey.Value as int)
	endif
	
EndEvent

Event OnOptionInputAccept(int option, string value)
	;Fill input box with current value
	if (option == PTS_KeyCode)
		cVal = value as Int
		VOX_PushToSpeak.SetValue(value as int)
	
		SetInputOptionValue(PTS_KeyCode, cVal)
		SetKeyMapOptionValue(cOID, cVal)
		
	elseif (option == ShoutKey)
		VOX_ShoutKey.SetValue(value as int)
	
		SetInputOptionValue(ShoutKey, VOX_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, VOX_ShoutKey.Value as int)
	endif
	
EndEvent

Event OnOptionDefault(int option)
	;Fill input box with current value
	if (option == PTS_KeyCode || option == cOid)
		cVal = 44
		VOX_PushToSpeak.SetValue(cVal)
	
		SetInputOptionValue(PTS_KeyCode, cVal)
		SetKeyMapOptionValue(cOID, cVal)
		
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
		
	endif

EndEvent

Event OnOptionHighlight(int option)
	if (option == aOID)
		SetInfoText("Enable/Disable Voice Recognition")
		
	elseif (option == bOID)
		SetInfoText("Enable/Disable Push-to-Speak\nYou must specify the key in the space below (DOES NOT WORK FOR VR)")
		
	elseif (option == cOID)
		SetInfoText("The key you press to temporarilly enable Voice Recognition")
	
	elseif (option == VPTS_OID)
		SetInfoText("Say designated start listening and stop listening commands to enable/disable recognition. Default is \"Stat/Stop Listening\"")
		
	elseif (option == dOID)
		SetInfoText("Enable/Disable auto-casting powers after they are equipped by voice recognition")
		
	elseif (option == eOID)
		SetInfoText("Enable/Disable auto-casting shouts after they are equipped by voice recognition")
		
	elseif (option == SKey_OID)
		SetInfoText("Default: Set the key to the game's most common shout keybind, Z")
		
	elseif (option == Local_UpdateInterval)
		SetInfoText("The time in seconds the game will wait before checking for an update from the voice recognition program.\nHigher values lead to longer wait times after each command")
		
	elseif (option == fOID)
		SetInfoText("Enable/Disable the log in the top left corner after a command is recognized")
		
	elseif (option == longAutoCastOID)
		SetInfoText("This enables auto-casting of spells with long cast times, such as \"Frost Thrall\" and \"Mass Paralysis\".\nThis is disabled by default for balancing purposes")
		
	elseif (option == sensitivity_OID)
		SetInfoText("Adjusts how sensitive the Voice Recognition is. (1-100 from least to most sensitive)\nThis can help if commands aren't recognized easily or if breathing/talking activates commands")
	
	endIf
	
EndEvent





