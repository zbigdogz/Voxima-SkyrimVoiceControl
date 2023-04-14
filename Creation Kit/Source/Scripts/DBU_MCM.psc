Scriptname DBU_MCM extends SKI_ConfigBase

;Global Variables
GlobalVariable Property DBU_Enabled auto
GlobalVariable Property DBU_UpdateInterval auto
GlobalVariable Property DBU_PushToSpeak auto
GlobalVariable Property DBU_VocalPushToSpeak auto
GlobalVariable Property DBU_AutoCastPowers auto
GlobalVariable Property DBU_AutoCastShouts auto
GlobalVariable Property DBU_ShoutKey auto
GlobalVariable Property DBU_ShowLog auto
GlobalVariable Property DBU_LongAutoCast auto
GlobalVariable Property DBU_Sensitivity auto
GlobalVariable Property DBU_CheckForUpdate auto

String SkyrimInfoLocation = "../../../SkyrimInfo"

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
int sensitivityDefaultValue = 90
int sensitivityValue = 90
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
	
	JsonUtil.Load(SkyrimInfoLocation)
	
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
		SKey_OID = AddKeyMapOption("Shout Key", DBU_ShoutKey.Value as int, OPTION_FLAG_DISABLED)
		ShoutKey = AddInputOption("Shout Key", DBU_ShoutKey.Value as int, OPTION_FLAG_DISABLED)
	else
		SKey_OID = AddKeyMapOption("Shout Key", DBU_ShoutKey.Value as int, OPTION_FLAG_NONE)
		ShoutKey = AddInputOption("Shout Key", DBU_ShoutKey.Value as int, OPTION_FLAG_NONE)
	endif

	SetCursorPosition(1) ; Move cursor to top right position

	;Update Interval
	AddHeaderOption("Other")
	Local_UpdateInterval = AddSliderOption("Update Interval", DBU_UpdateInterval.Value, "Every {2} Seconds")

	sensitivity_OID = AddSliderOption("Voice Recognition", sensitivityValue, "{2}% Sensitivity")	;The value is saved locally because otherwise, it may not save instantly
	
	;Show Log
	fOID = AddToggleOption("Show Log", fVal)
	
	;Check for Update
	CheckUpdate = AddToggleOption("Check for Update", DBU_CheckForUpdate.Value as bool)
	
	;Long Auto-Cast Spells
	longAutoCastOID = AddToggleOption("Auto-Cast Long Spells", DBU_LongAutoCast.Value as bool)
	
EndEvent

Event OnOptionSelect(int option)

	if (option == aOID)
		aVal = !aVal
		DBU_Enabled.SetValue(aVal as Int)
		SetToggleOptionValue(aOID, aVal)
		
	;Push-To-Speak Toggle
	elseif (option == bOID)
		bVal = !bVal 
		SetToggleOptionValue(bOID, bVal)
		
		if (!bVal)
			DBU_PushToSpeak.SetValue(-1)
			SetOptionFlags(cOID, OPTION_FLAG_DISABLED)
			SetOptionFlags(PTS_KeyCode, OPTION_FLAG_DISABLED)
		else
			DBU_PushToSpeak.SetValue(cVal)
			SetOptionFlags(cOID, OPTION_FLAG_NONE)
			SetOptionFlags(PTS_KeyCode, OPTION_FLAG_NONE)
		endif
	
	;Vocal Push-to-Speak
	elseif (option == VPTS_OID)
		VPTS_Val = !VPTS_Val
		DBU_CheckForUpdate.SetValue(1)
		DBU_VocalPushToSpeak.SetValue(VPTS_Val as int)
		
		JsonUtil.SetPathStringValue(SkyrimInfoLocation, "stats.vocalpts", DBU_VocalPushToSpeak.GetValue() as int)
		SetToggleOptionValue(VPTS_OID, VPTS_Val)
			
	;Auto-Cast Powers
	elseif (option == dOID)
		dVal = !dVal
		DBU_AutoCastPowers.SetValue(dVal as int)
		JsonUtil.SetPathIntValue(SkyrimInfoLocation, ".stats.autocastpowers", DBU_AutoCastPowers.Value as Int)
		
		
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
		DBU_AutoCastShouts.SetValue(eVal as int)
		SetToggleOptionValue(eOID, eVal)
		JsonUtil.SetPathIntValue(SkyrimInfoLocation, ".stats.autocastshouts", DBU_AutoCastShouts.Value as Int)
		
		if (!dVal && !eVal)
			SetOptionFlags(ShoutKey, OPTION_FLAG_DISABLED)
			SetOptionFlags(SKey_OID, OPTION_FLAG_DISABLED)
		else
			SetOptionFlags(ShoutKey, OPTION_FLAG_NONE)
			SetOptionFlags(SKey_OID, OPTION_FLAG_NONE)
		endif
		
	elseif (option == fOID)
		fVal = !fVal
		DBU_ShowLog.SetValue(fVal as int)
		SetToggleOptionValue(fOID, fVal)
		
	elseif (option == CheckUpdate)
		DBU_CheckForUpdate.SetValue(1)
		SetToggleOptionValue(CheckUpdate, DBU_CheckForUpdate.Value as bool)
		
	elseif (option == longAutoCastOID)
		DBU_LongAutoCast.SetValue((!(DBU_LongAutoCast.Value as bool)) as int)
		SetToggleOptionValue(longAutoCastOID, DBU_LongAutoCast.Value as bool)
	endif
	
	JsonUtil.Unload(SkyrimInfoLocation)
EndEvent

Event OnOptionKeyMapChange(int option, int keyCode, string conflictControl, string conflictName)
	if (option == cOID)
		cVal = keyCode
		SetKeyMapOptionValue(cOID, cVal)
		SetInputOptionValue(PTS_KeyCode, cVal)
		DBU_PushToSpeak.SetValue(keyCode)
	
	elseif (option == SKey_OID)
		DBU_ShoutKey.SetValue(keyCode)
		SetKeyMapOptionValue(SKey_OID, DBU_ShoutKey.Value as int)
		SetInputOptionValue(ShoutKey, DBU_ShoutKey.Value as int)
	
	endIf
EndEvent

;Sliders
Event OnOptionSliderOpen(int option)
	if (option == Local_UpdateInterval)
		SetSliderDialogStartValue(DBU_UpdateInterval.Value)
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
		
		DBU_UpdateInterval.SetValue(value)
		
		if (value <= 1.0)
			SetSliderOptionValue(Local_UpdateInterval, DBU_UpdateInterval.Value, "Every {2} Second")
			
		else
			SetSliderOptionValue(Local_UpdateInterval, DBU_UpdateInterval.Value, "Every {2} Seconds")
			
		endif
	elseif (option == sensitivity_OID)
		
		sensitivityValue = value as int
		DBU_Sensitivity.SetValue(sensitivityValue)
		
		SetSliderOptionValue(sensitivity_OID, value as int, "{2}% Sensitivity")
		
	endif

EndEvent

Event OnOptionInputOpen(int option)
	;Fill input box with current value
	if (option == PTS_KeyCode)
		SetInputDialogStartText(cVal)
		
	elseif (option == ShoutKey)
		SetInputDialogStartText(DBU_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, DBU_ShoutKey.Value as int)
	endif
	
EndEvent

Event OnOptionInputAccept(int option, string value)
	;Fill input box with current value
	if (option == PTS_KeyCode)
		cVal = value as Int
		DBU_PushToSpeak.SetValue(value as int)
	
		SetInputOptionValue(PTS_KeyCode, cVal)
		SetKeyMapOptionValue(cOID, cVal)
		
	elseif (option == ShoutKey)
		DBU_ShoutKey.SetValue(value as int)
	
		SetInputOptionValue(ShoutKey, DBU_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, DBU_ShoutKey.Value as int)
	endif
	
EndEvent

Event OnOptionDefault(int option)
	;Fill input box with current value
	if (option == PTS_KeyCode || option == cOid)
		cVal = 44
		DBU_PushToSpeak.SetValue(cVal)
	
		SetInputOptionValue(PTS_KeyCode, cVal)
		SetKeyMapOptionValue(cOID, cVal)
		
	elseif (option == ShoutKey || option == SKey_OID)
		DBU_ShoutKey.SetValue(44)	;Input.GetMappedKey("Shout") gives the default key but this doesn't work on VR. value 44 is Z, which is the most common
	
		SetInputOptionValue(ShoutKey, DBU_ShoutKey.Value as int)
		SetKeyMapOptionValue(SKey_OID, DBU_ShoutKey.Value as int)
	
	elseif (option == sensitivity_OID)
		
		sensitivityValue = sensitivityDefaultValue
		DBU_Sensitivity.SetValue(sensitivityValue)
		
		SetSliderOptionValue(sensitivity_OID, sensitivityValue,"{2}% Sensitivity")
	
	elseif (option == longAutoCastOID)
		DBU_LongAutoCast.SetValue(0)
		SetToggleOptionValue(longAutoCastOID, DBU_LongAutoCast.Value as bool)
		
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





