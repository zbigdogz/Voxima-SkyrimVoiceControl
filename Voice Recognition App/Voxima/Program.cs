// TODO:
// Update the code marked with *** in comments
// more testing for changing the port on the fly, as well as providing bad port information

// zbigdogz VOX build path => ..\..\..\..\..\..\Games\Skyrim\MO2\Appdata\SkyrimSE\mods\Voxima\SKSE\Plugins
// exergist VOX build path => G:\Modding\Tools\Mod Organizer 2 - Games\Skyrim Special Edition\mods\Voxima\SKSE\Plugins\VOX\Speech Recognition Application\

using SpeechDictionaryTools;
using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Resources;
using System.Runtime.InteropServices;
using System.Speech.Recognition;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using Utilities;
using WebSocket;
using WebsocketPortConfigurator;

namespace Voxima
{
    internal class Program
    {

        #region Fields

        static SpeechRecognitionEngine recognizer = new SpeechRecognitionEngine(System.Globalization.CultureInfo.CurrentCulture);
        static string ip = "localhost"; // Define websocket server target IP address (use localhost in this case, which is 127.0.0.1)
        static int defaultWebsocketPort = 19223; // Define default target websocket port
        static int websocketPort = -1;
        static FileSystemWatcher portConfigWatcher;
        //static bool runApplication = true;
        static ObservableConcurrentQueue<string> observableConcurrentQueue;
        static DictionaryInterface dictionaryInterface;
        static readonly System.Threading.EventWaitHandle waitHandle = new System.Threading.AutoResetEvent(false);
        static ResourceManager dovahzulPhonemes = new ResourceManager("Voxima.DovahzulPhonemes", Assembly.GetExecutingAssembly());
        static List<string> trainedWords = new List<string>();


        //Addresses;

        static string PlayerInfoAddress;

        static string GameInfoAddress;
        static string SpellsAddress;
        static string ShoutsAddress;
        static string PowersAddress;
        static string KeybindsAddress;
        static string ProgressionsAddress;
        static string VSettingsAddress;
        static string ConsoleCommandsAddress;
        static string RunCheck; //The presence of this file means this program has run before
        static string oldPapyrusCheck;

        //Media Addresses
        static System.Media.SoundPlayer sound_start_listening;
        static System.Media.SoundPlayer sound_stop_listening;

        static string WerewolfKeybindsAddress;
        static string VampireLordSpellsAddress;
        static string VampireLordPowersAddress;
        static string VampireLordKeybindsAddress;
        static string VampireLordProgressionsAddress;


        static string DragonKeybindsAddress;

        static string HorseKeybindsAddress;

        static string ActivityDebugAddress;
        static string SettingsAddress;

        static string ConflictsAddress;
        static string SavedConflictsAddress;

        //Log
        static string PreviousLog = "";
        static string CurrentLog = "";

        //Files
        static string[] SpellFiles;
        static string[] ShoutFiles;
        static string[] PowerFiles;
        static string[] KeybindFiles;
        static string[] ProgressionFiles;

        static string[] VSettingFiles;
        static string[] ConsoleCommandFiles;

        static string[] WerewolfKeybindFiles;
        static string[] VampireLordSpellFiles;
        static string[] VampireLordPowerFiles;
        static string[] VampireLordKeybindFiles;
        static string[] VampireLordProgressionFiles;

        static string[] DragonKeybindFiles;

        static string[] HorseKeybindFiles;

        //.ini Lists in Files
        static readonly int MaxNumFiles = 50;

        static List<string> SpellList = new List<string>();
        static List<string> ShoutList = new List<string>();
        static List<string> PowerList = new List<string>();
        static List<string> KeybindList = new List<string>();
        static List<string> ProgressionList = new List<string>();
        static List<string> VSettingList = new List<string>();
        static List<string> ConsoleCommandList = new List<string>();

        static List<string> WerewolfKeybindList = new List<string>();
        static List<string> VampireLordSpellList = new List<string>();
        static List<string> VampireLordPowerList = new List<string>();
        static List<string> VampireLordKeybindList = new List<string>();
        static List<string> VampireLordProgressionList = new List<string>();

        static List<string> DragonKeybindList = new List<string>();
        static List<string> HorseKeybindList = new List<string>();

        //Settings
        static string DefaultHand = "";
        static string AutoStart = "";
        static string StartWithComputer = "";
        static float Sensitivity = 0.1F;
        static string RequireHandSelector = "";
        static bool AutoCastPowers = false;
        static bool AutoCastShouts = false;
        static bool VocalPushToSpeak = false;     //Adjusted in-game through the MCM
        static bool VocalPushToSpeak_Val = true;     //Stored Value

        static List<Grammar> SettingGrammars = new List<Grammar>();
        static List<Grammar> KeybindGrammars = new List<Grammar>();
        static List<Grammar> ConsoleGrammars = new List<Grammar>();
        static Hashtable LocationsGrammars = new Hashtable();
        static Hashtable AllItems = new Hashtable();
        static Hashtable AllConflicts = new Hashtable();
        static string DuplicatesText = "";
        static Hashtable AllIDs = new Hashtable();
        static Hashtable AllVampireLordIDs = new Hashtable();
        static Hashtable AllWerewolfIDs = new Hashtable();
        static Hashtable AllProgressions = new Hashtable();
        static Hashtable AllProgressionItems = new Hashtable();
        static string[] KnownSpells;
        static string[] KnownShouts;
        static string[] KnownPowers;
        static string[] KnownLocations;
        static string[] CurrentSpells = new string[1] { null };
        static string[] CurrentShouts = new string[1] { null };
        static string[] CurrentPowers = new string[1] { null };
        static string CurrentMorph = "";
        static DictationGrammar FullDictation = new DictationGrammar();

        static List<string> handLeft = new List<string>();
        static List<string> handRight = new List<string>();
        static List<string> handBoth = new List<string>();
        static List<string> handCast = new List<string>();
        static List<string> powerCast = new List<string>();
        static List<string> locationCommands = new List<string>();
        static List<string> currentLocationCommands = new List<string>();

        static string Morph = "none";
        static DateTime Time = DateTime.Now;
        static int updateNum = 0;
        static bool isUpdating = false;

        static int commandNum = 1;

        static WebSocketServer server;

        #endregion Fields

        static void Main(string[] args)
        {
            MessageBox.Show("Waiting for confirmation that a debugger has been attatcehd", "C# Voice Recognition Application", MessageBoxButtons.OK, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, (MessageBoxOptions)0x40000);


            //Initialize the Global Variables\\
            //Addresses

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Commands"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX\Commands");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Commands\Special"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX\Commands\Special");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Logs"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX\Logs");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Logs\VoximaApp.log"))
                System.IO.File.WriteAllText(@"Data\SKSE\Plugins\VOX\Logs\VoximaApp.log", "");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Debug"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX\Debug");

            SpellsAddress = @"Data\SKSE\Plugins\VOX\Commands\Spells";
            if (!System.IO.File.Exists(SpellsAddress))
                System.IO.Directory.CreateDirectory(SpellsAddress);

            ShoutsAddress = @"Data\SKSE\Plugins\VOX\Commands\Shouts";
            if (!System.IO.File.Exists(ShoutsAddress))
                System.IO.Directory.CreateDirectory(ShoutsAddress);

            PowersAddress = @"Data\SKSE\Plugins\VOX\Commands\Powers";
            if (!System.IO.File.Exists(PowersAddress))
                System.IO.Directory.CreateDirectory(PowersAddress);

            KeybindsAddress = @"Data\SKSE\Plugins\VOX\Commands\Keybinds";
            if (!System.IO.File.Exists(KeybindsAddress))
                System.IO.Directory.CreateDirectory(KeybindsAddress);

            ProgressionsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Progressions";
            if (!System.IO.File.Exists(ProgressionsAddress))
                System.IO.Directory.CreateDirectory(ProgressionsAddress);

            VSettingsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Settings";     //Voiced Settings
            if (!System.IO.File.Exists(VSettingsAddress))
                System.IO.Directory.CreateDirectory(VSettingsAddress);

            GameInfoAddress = @"Data\SKSE\Plugins\VOX\Game Information";     //Voiced Settings
            if (!System.IO.File.Exists(GameInfoAddress))
                System.IO.Directory.CreateDirectory(GameInfoAddress);

            PlayerInfoAddress = @"Data\SKSE\Plugins\VOX\Game Information\PlayerInfo.txt";
            if (!System.IO.File.Exists(PlayerInfoAddress))
                System.IO.File.WriteAllText(PlayerInfoAddress, "");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Commands"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX\Commands");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Commands\Special"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX\Commands\Special");

            if (!System.IO.File.Exists(@"Data\SKSE\Plugins\VOX\Debug"))
                System.IO.Directory.CreateDirectory(@"Data\SKSE\Plugins\VOX\Debug");

            SpellsAddress = @"Data\SKSE\Plugins\VOX\Commands\Spells";
            if (!System.IO.File.Exists(SpellsAddress))
                System.IO.Directory.CreateDirectory(SpellsAddress);

            ShoutsAddress = @"Data\SKSE\Plugins\VOX\Commands\Shouts";
            if (!System.IO.File.Exists(ShoutsAddress))
                System.IO.Directory.CreateDirectory(ShoutsAddress);

            PowersAddress = @"Data\SKSE\Plugins\VOX\Commands\Powers";
            if (!System.IO.File.Exists(PowersAddress))
                System.IO.Directory.CreateDirectory(PowersAddress);

            KeybindsAddress = @"Data\SKSE\Plugins\VOX\Commands\Keybinds";
            if (!System.IO.File.Exists(KeybindsAddress))
                System.IO.Directory.CreateDirectory(KeybindsAddress);

            ProgressionsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Progressions";
            if (!System.IO.File.Exists(ProgressionsAddress))
                System.IO.Directory.CreateDirectory(ProgressionsAddress);

            VSettingsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Settings";     //Voiced Settings
            if (!System.IO.File.Exists(VSettingsAddress))
                System.IO.Directory.CreateDirectory(VSettingsAddress);

            ConsoleCommandsAddress = @"Data\SKSE\Plugins\VOX\Commands\Console";     //Console Commands
            if (!System.IO.File.Exists(ConsoleCommandsAddress))
                System.IO.Directory.CreateDirectory(ConsoleCommandsAddress);


            //Media Addresses
            sound_start_listening = new System.Media.SoundPlayer(@"Data\SKSE\Plugins\VOX\Media\Start listening.wav");
            sound_stop_listening = new System.Media.SoundPlayer(@"Data\SKSE\Plugins\VOX\Media\Stop listening.wav");

            //Werewolf
            WerewolfKeybindsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Werewolf\Keybinds";
            if (!System.IO.File.Exists(WerewolfKeybindsAddress))
                System.IO.Directory.CreateDirectory(WerewolfKeybindsAddress);

            //Vampire Lord
            VampireLordSpellsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Vampire Lord\Spells";
            if (!System.IO.File.Exists(VampireLordSpellsAddress))
                System.IO.Directory.CreateDirectory(VampireLordSpellsAddress);

            VampireLordPowersAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Vampire Lord\Powers";
            if (!System.IO.File.Exists(VampireLordPowersAddress))
                System.IO.Directory.CreateDirectory(VampireLordPowersAddress);

            VampireLordKeybindsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Vampire Lord\Keybinds";
            if (!System.IO.File.Exists(VampireLordKeybindsAddress))
                System.IO.Directory.CreateDirectory(VampireLordKeybindsAddress);

            VampireLordProgressionsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Vampire Lord\Progressions";
            if (!System.IO.File.Exists(VampireLordProgressionsAddress))
                System.IO.Directory.CreateDirectory(VampireLordProgressionsAddress);

            //Dragon Riding
            DragonKeybindsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Dragon Riding\Keybinds";
            if (!System.IO.File.Exists(DragonKeybindsAddress))
                System.IO.Directory.CreateDirectory(DragonKeybindsAddress);

            //Horse Riding
            HorseKeybindsAddress = @"Data\SKSE\Plugins\VOX\Commands\Special\Horse Riding\Keybinds";
            if (!System.IO.File.Exists(HorseKeybindsAddress))
                System.IO.Directory.CreateDirectory(HorseKeybindsAddress);

            ActivityDebugAddress = @"Data\SKSE\Plugins\VOX\Debug\Activity.txt";
            if (!System.IO.File.Exists(ActivityDebugAddress))
                System.IO.File.WriteAllText(ActivityDebugAddress, "");


            SavedConflictsAddress = ProgressionsAddress + @"\SavedConflicts.ini";
            if (!System.IO.File.Exists(SavedConflictsAddress))
                System.IO.File.WriteAllText(SavedConflictsAddress, "");


            string defaultSettings = "This is where you can change the program's settings manually.\n" +
                                        "Note: Changing settings manually requires you to restart the program before they take effect.\n\n" +

                                        "Options for each setting:\n" +
                                            "\tDefault Hand = \"Left\", \"Right\", \"Both\"\n" +
                                            "\tAutoStart = \"True\", \"False\"\t\t//Runs the program in the background when Skyrim is not open and speech recognition is NOT enabled.\n" +
                                            "\tStart With Computer = \"True\", \"False\"\n" +
                                            "\tSensitivity = {0 : 1}\t//Controls how sensitive the system is to commands. 0 is very sensitive, one is least sensitive, and 0.5 is in-between\n\n" +

                                            "Change the settings below.\n" +
                                        "--------------------------------------------------\n\n" +

                                        "Default Hand = Left\n" +
                                        "AutoStart = True\n" +
                                        "Start With Computer = True\n" +
                                        "Require Hand Selector = False\n";

            SettingsAddress = @"Data\SKSE\Plugins\VOX\Settings.txt";
            if (!System.IO.File.Exists(SettingsAddress))
                System.IO.File.WriteAllText(SettingsAddress, defaultSettings);

            RunCheck = @"Data\SKSE\Plugins\VOX\Debug\RunCheck";
            oldPapyrusCheck = @"Data\SKSE\Plugins\VOX\Debug\oldPapyusUtilCheck";

            //Delete the file incase it isn't needed
            ConflictsAddress = ProgressionsAddress + @"\Conflicts.ini";
            if (System.IO.File.Exists(ConflictsAddress))
                System.IO.File.Delete(ConflictsAddress);

            #region Log File Setup

            //log.InitializeLog(logOutput); // Initialize log file
            Log.InitializeLogs();

            #endregion

            #region Skyrim Process Exit Monitoring

            string[] targetProcesses = new string[] { "SkyrimSE", "SkyrimVR", "Notepad"};
            if (ProcessWatcher.MonitorForExit(targetProcesses) == true) // Call method to monitor the targetProcesses for exit in separate thread (actually the first of the targetProcesses found), and check if processing was successful
                ProcessWatcher.ProcessClosed += OnProcessClosed; // Subscribe OnProcessClosed to ProcessClosed events

            #endregion

            #region Websocket Port Configuration

            ConfigureWebsocketPort(); // Read initial info from and watch for changes in PortConfig.txt file (populated by SKSE plugin)
            /*//debug1//*/
            while (websocketPort == -1)
            {
                Log.Debug("Waiting for websocket port configuration...", Log.LogType.Info);
                Thread.Sleep(500);
            }
            /*//debug2//*/

            #endregion

            #region Add Voxima Content to Speech Dictionary

            /* #region Demo

            // Create DictionaryInterface instance so that speech dictionary modifications and other processing can be performed
            DictionaryInterface dictionaryInterface = new DictionaryInterface();

            // Here are some untrained dragon shouts
            dictionaryInterface.VoiceText("Fus, Ro, Dah");
            dictionaryInterface.VoiceText("Iiz, Slen, Nus");
            dictionaryInterface.VoiceText("Tiid, Klo, Ul");

            Thread.Sleep(500);

            // Now let's teach the speech dictionary dragon shouts
            dictionaryInterface.ModifyDictionaryDataAsync(DictionaryInterface.DictionaryAction.Add, false, "Skyrim Dictionary Training.txt", true);

            Thread.Sleep(500);

            // Listen to the dragon shouts after dictionary training
            dictionaryInterface.VoiceText("Fus, Ro, Dah");
            dictionaryInterface.VoiceText("Iiz, Slen, Nus");
            dictionaryInterface.VoiceText("Tiid, Klo, Ul");

            Thread.Sleep(500);

            // Now let's remove the dragon shout words from the speech dictionary
            dictionaryInterface.ModifyDictionaryDataAsync(DictionaryInterface.DictionaryAction.Remove, false, "Skyrim Dictionary Training.txt", true);

            #endregion */

            // Create DictionaryInterface instance so that speech dictionary modifications and other processing can be performed
            dictionaryInterface = new DictionaryInterface();

            // Now let's teach the speech dictionary dragon shouts
            dictionaryInterface.ModifyDictionaryDataAsync(DictionaryInterface.DictionaryAction.Add, false, "Skyrim Dictionary Training.txt", true);

            #endregion

            #region WebSocket Server Initialization

            /*//debug1//*/
            StartWebsocketServer(ip, websocketPort); // Initialize and launch websocket server

            ///LaunchPortConfigurator(); // Launch user interface (window) for specifying the target websocket server port

            // Loop to wait for client connection (debug)
            int counter = 0;
            int logInterval = 10;
            int waitInterval = 500; //in milliseconds
            while (server?.IsClientConnected == false)
            {
                if ((double)counter / logInterval == counter / logInterval)
                    Log.Debug($"Waiting for client connection... ({counter * logInterval * waitInterval / 1000}ms)", Log.LogType.Info);
                counter++;
                Thread.Sleep(waitInterval); // Brief pause
            }
            /*//debug2//*/


            #endregion

            #region Message Queueing Setup

            observableConcurrentQueue = new ObservableConcurrentQueue<string>();
            observableConcurrentQueue.CollectionChanged += OnObservableConcurrentQueueCollectionChanged;

            #endregion

            /* #region Application Variable Setup (demo for now)

            // Demo for sending a message to WebSocketClient
            while (server != null) {
                if (server.IsClientConnected == true) {
                    while (server != null) {
                        Random r = new Random();
                        int number = r.Next(0, 100);
                        server.SendMessage($"Hello World! {number}");
                        Thread.Sleep(3000);
                    }
                    if (server == null)
                        break;
                }
                Thread.Sleep(1000);
                Log.Debug("Waiting for client...", Log.LogType.Info);
            }

            #endregion */

            int i = 0, j = 0;

            /*//debug1//*/
            try
            {
                /*//debug2//*/

                #region Application Processing Initialization

                //Files
                SpellFiles = Directory.GetFiles(SpellsAddress);
                ShoutFiles = Directory.GetFiles(ShoutsAddress);
                PowerFiles = Directory.GetFiles(PowersAddress);
                KeybindFiles = Directory.GetFiles(KeybindsAddress);
                ProgressionFiles = Directory.GetFiles(ProgressionsAddress);

                VSettingFiles = Directory.GetFiles(VSettingsAddress);
                ConsoleCommandFiles = Directory.GetFiles(ConsoleCommandsAddress);

                WerewolfKeybindFiles = Directory.GetFiles(WerewolfKeybindsAddress);

                VampireLordSpellFiles = Directory.GetFiles(VampireLordSpellsAddress);
                VampireLordPowerFiles = Directory.GetFiles(VampireLordPowersAddress);
                VampireLordKeybindFiles = Directory.GetFiles(VampireLordKeybindsAddress);
                VampireLordProgressionFiles = Directory.GetFiles(VampireLordProgressionsAddress);

                DragonKeybindFiles = Directory.GetFiles(DragonKeybindsAddress);

                HorseKeybindFiles = Directory.GetFiles(HorseKeybindsAddress);

                //----------Initialization Finished----------\\

                #endregion

                #region User Input Gathering

                //Determine if the User has run this program before. If not, ask if they want to run speech recognition training
                if (!System.IO.File.Exists(RunCheck))
                {
                    string title = "Voxima Setup";
                    string message = "It seems you haven't used Voxima before. It is recommended that you set up your microphone and " +
                                     "run voice recognition training. Would you like to do this now?\n\n" +

                                     "Properly setting up your microphone and training your system allows for quicker and more accurate voice " +
                                     "recognition. It is recommended that you perform at least 3 rounds of speech recognition training.\n\n" +

                                     "If you select \"Yes,\" this program will terminate and launch the Speech Recognition menu, where you can " +
                                     "initiate microphone setup and then recognition training. Skyrim will need to be restarted before these changes " +
                                     "automatically apply to Voxima.\n\n" +

                                     "Once in the Speech Recognition menu, press \"Set up microphone\" and follow the instructions.\n\n" +

                                     "After microphone setup is complete, press \"Train your computer to better understand you\" and follow the instructions. " +
                                     "Again, going through at least 3 rounds of training is recommended.\n\n" +

                                     "If you select \"No,\" the microphone setup and speech recognition training will be skipped. " +
                                     "A shortcut to launch the Speech Recognition menu may also be found in \"Plugins\\VOX\\Debug.\"";

                    MessageBoxButtons buttons = MessageBoxButtons.YesNo;
                    ///DialogResult result = MessageBox.Show(message, title, buttons);
                    DialogResult result = MessageBox.Show(message, title, buttons, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, (MessageBoxOptions)0x40000);
                    if (result == DialogResult.Yes)
                    {
                        ///LaunchSpeechRecognitionTraining();
                        LaunchSpeechRecognitionMenu();
                        Thread.Sleep(250); // Brief pause

                        buttons = MessageBoxButtons.OK;
                        title = "VOX - Restart Skyrim";
                        message = "Remember to restart Skyrim for microphone setup and voice training changes to apply to Voxima.";
                        MessageBox.Show(message, title, buttons, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, (MessageBoxOptions)0x40000);
                        return;
                    }
                    System.IO.File.Create(RunCheck);
                }

                if (System.IO.File.Exists(oldPapyrusCheck))
                {
                    string message = "WARNING: the \"Campfire\" mod has an old version of PapyrusUtil included with the download.\n\n" +
                        "If using Mod Organizer 2, naviage to Campfire's mod folder, then to 'SKSE\\Plugins', then delete the 'PapyrusUtil.dll' file\n\n" +
                        "If using Vortex, go to your Skyrim Directory, navigate to 'SKSE\\Plugins', then replace the 'PapyrusUtil.dll' file with the one you downloaded from the Nexus\n\n" +
                        "Would you like to disable this warning?";
                    string title = "Voxima Voice Recognition";
                    MessageBoxButtons buttons = MessageBoxButtons.YesNo;
                    ///DialogResult result = MessageBox.Show(message, title, buttons, MessageBoxOptions.DefaultDesktopOnly);
                    DialogResult result = MessageBox.Show(message, title, buttons, MessageBoxIcon.None, MessageBoxDefaultButton.Button1, (MessageBoxOptions)0x40000);

                    if (result == DialogResult.Yes)
                        System.IO.File.Delete(oldPapyrusCheck);
                }

                #endregion

                #region Application Setup

                //Fill Global Variables
                PreviousLog = System.IO.File.ReadAllText(ActivityDebugAddress);
                //Load Settings
                string SettingsList = System.IO.File.ReadAllText(SettingsAddress);
                string SettingName;

                for (; i < SettingsList.Length && SettingsList[i] != '-'; i++) ; //Skip to the dashed line

                for (; i < SettingsList.Length && SettingsList[i - 1] != '\n'; i++) ; //Skip to the end of the line

                SettingName = "Default Hand";
                for (; i < SettingsList.Length; i++)
                { //Find "Default Hand"
                    for (j = 0; j < SettingName.Length && SettingsList[i] == SettingName[j]; i++, j++) ;

                    if (j >= SettingName.Length)
                    {
                        i += 3; //Skip " = "
                        for (; SettingsList[i] != '\n' && SettingsList[i] != '\r' && i < SettingsList.Length; i++)
                            DefaultHand += SettingsList[i];

                        Log.Activity("Default Hand set to \"" + DefaultHand + "\"");

                        break;
                    }
                    else
                        i -= j - 1;

                }

                for (; i < SettingsList.Length && SettingsList[i] != '\n'; i++) ;

                SettingName = "AutoStart";
                for (i++; i < SettingsList.Length; i++)
                {
                    for (j = 0; j < SettingName.Length && SettingsList[i] == SettingName[j]; i++, j++) ;

                    if (j >= SettingName.Length)
                    {
                        i += 3; //Skip " = "
                        for (; SettingsList[i] != '\n' && SettingsList[i] != '\r' && i < SettingsList.Length; i++)
                            AutoStart += SettingsList[i];

                        Log.Activity("AutoStart set to \"" + AutoStart + "\"");

                        break;
                    }
                    else
                        i -= j - 1;

                }

                for (; i < SettingsList.Length && SettingsList[i] != '\n'; i++) ;

                SettingName = "Start With Computer";
                for (i++; i < SettingsList.Length; i++)
                {
                    for (j = 0; j < SettingName.Length && SettingsList[i] == SettingName[j]; i++, j++) ;

                    if (j >= SettingName.Length)
                    {
                        i += 3; //Skip " = "
                        for (; SettingsList[i] != '\n' && SettingsList[i] != '\r' && i < SettingsList.Length; i++)
                            StartWithComputer += SettingsList[i];

                        Log.Activity("Start With computer set to \"" + StartWithComputer + "\"");

                        break;
                    }
                    else
                        i -= j - 1;

                }

                for (; i < SettingsList.Length && SettingsList[i] != '\n'; i++) ;

                SettingName = "Require Hand Selector";
                for (i++; i < SettingsList.Length; i++)
                {
                    for (j = 0; j < SettingName.Length && SettingsList[i] == SettingName[j]; i++, j++) ;

                    if (j >= SettingName.Length)
                    {
                        i += 3; //Skip " = "
                        for (; SettingsList[i] != '\n' && SettingsList[i] != '\r' && i < SettingsList.Length; i++)
                            RequireHandSelector += SettingsList[i];

                        Log.Activity("Require Hand Selector set to \"" + RequireHandSelector + "\"");

                        break;
                    }
                    else
                        i -= j - 1;

                }

                Log.Activity("Voice Recognition set to \"" + System.Globalization.CultureInfo.CurrentCulture + "\"");

                Log.Activity("--------------------\n");

                if (DefaultHand == "")
                {
                    Log.Activity("There is an error in the settings file causing \"DefaultHand\" to have no result", Log.LogType.Error);
                }

                if (AutoStart == "")
                {
                    Log.Activity("There is an error in the settings file causing \"AutoStart\" to have no result", Log.LogType.Error);
                }

                if (StartWithComputer == "")
                {
                    Log.Activity("There is an error in the settings file causing \"StartWithComputer\" to have no result", Log.LogType.Error);
                }

                if (RequireHandSelector == "")
                {
                    Log.Activity("There is an error in the settings file causing \"RequireHandSelector\" to have no result", Log.LogType.Error);
                }

                //Get Items

                FindCommands(VSettingFiles, "setting", "setting");                                //Get Command Settings and Start/Stop commands
                FindCommands(ConsoleCommandFiles, "console", "None");                       //Get Console commands

                FindCommands(VampireLordPowerFiles, "Power", "VampireLord");              //Get Vampire Lord Powers
                FindCommands(VampireLordSpellFiles, "Spell", "VampireLord");              //Get Vampire Lord Spells

                FindCommands(PowerFiles, "Power", "None");                                           //Get Powers
                FindCommands(SpellFiles, "Spell", "None");                                           //Get Spells

                FindCommands(ShoutFiles, "Shout", "None");                                           //Get Shouts

                FindCommands(DragonKeybindFiles, "Keybind", "DragonRiding",                 //Get Dragon Riding Keybinds
                FindCommands(KeybindFiles, "Keybind", "None",                                     //Get None Keybinds
                FindCommands(WerewolfKeybindFiles, "Keybind", "Werewolf",                 //Get Werewolf Keybinds
                FindCommands(VampireLordKeybindFiles, "Keybind", "VampireLord",        //Get Vampire Lord Keybinds
                FindCommands(HorseKeybindFiles, "Keybind", "HorseRiding", 0)))));            //Get Horse Riding Commands

                j = FindProgression(ProgressionFiles, "None",                                  //Get Progressions
                FindProgression(VampireLordProgressionFiles, "VampireLord", 0));    //Get Vampire Lord Progressions

                //Get "Current Location" commands
                Choices commands = new Choices();
                foreach (string item in currentLocationCommands)
                {
                    if (item == null)
                        break;

                    commands.Add(item);
                }

                if (currentLocationCommands[0] != null)
                {
                    Grammar grammar = new Grammar(commands);
                    grammar.Name = $"playerlocation\tlocation";
                    LocationsGrammars.Add("playerlocation", grammar);
                }


                //Create/Fill Conflicts progressions file
                string[] allValues = new string[AllConflicts.Count];
                string allConflictsText = "";

                AllConflicts.Values.CopyTo(allValues, 0);

                for (i = 0; i < AllConflicts.Count; i++)
                {
                    allConflictsText += allValues[i];
                }

                System.IO.File.WriteAllText(ConflictsAddress, allConflictsText);

                //This is a rough and remporary fix. This needs to be changed to happen before the progressions and ProgressionFiles are gotten
                ProgressionFiles = new string[1];
                ProgressionFiles[0] = ConflictsAddress;

                if (DuplicatesText != "")
                    Log.Activity("\nThe following items are duplicate entires or conflicting commands within the same mod:\n" + DuplicatesText + "\nPlease report this list so that I may fix it. These items may not work properly or at all until they are fixed\n");

                FindProgression(ProgressionFiles, "None", j);


                // Add a handler for the speech recognized event.  
                recognizer.SpeechRecognized += new EventHandler<SpeechRecognizedEventArgs>(Recognizer_SpeechRecognized);
                //recognizer.AudioLevelUpdated += new EventHandler<AudioLevelUpdatedEventArgs>(Recognizer_AudioLevelUpdated);   //See function for details on this line

                //Set recognition specifications(same settings as Voice Macro)
                recognizer.BabbleTimeout = TimeSpan.FromMilliseconds(50);
                recognizer.InitialSilenceTimeout = TimeSpan.FromSeconds(5);
                recognizer.EndSilenceTimeout = TimeSpan.FromMilliseconds(50);
                recognizer.EndSilenceTimeoutAmbiguous = TimeSpan.FromMilliseconds(50);
                recognizer.MaxAlternates = 1;

                // Configure input to the speech recognizer.  
                recognizer.SetInputToDefaultAudioDevice();

                FullDictation.Name = "FullDictation";

                if (!(Sensitivity >= 0 && Sensitivity <= 1))
                    Sensitivity = 0.9f;

                FullDictation.Weight = Sensitivity;

                #endregion

                #region Main Processing

                Log.Activity("Program Running");
            ChangeCommands(Morph);
                waitHandle.WaitOne(); // Wait main thread at this line until wait restriction is removed (set)

                //bool found = false;
                ////Check if the game is running 
                //foreach (string name in targetProcesses)
                //{
                //    if (Process.GetProcessesByName(name).Length >= 1)
                //        found = true;
                //}

                //if (found)
                //{
                //    Log.Activity("Program Running");
                //    //ChangeCommands(Morph);    //This is used for debuging without websocket
                //    //int count = 0;
                //    /* while (runApplication == true)
                //    {
                //        //if (count == 5)
                //        //{
                //        //    count = 0;
                //        //    ChangeCommands(Morph);
                //        //}
                //        Thread.Sleep(1000);
                //        //count += 1;
                //    }//End while */
                //    //Game is Not Running
                //} else
                //{
                //    Log.Activity("Program Closed because Skyrim is not running");
                //}//End if else

                #endregion

                /*//debug1//*/
            }
            catch (Exception ex)
            {
                Log.Debug($"Error during VOX application execution: {ex.Message}", Log.LogType.Error);
                Log.Activity(ex.ToString());
            }
            /*//debug2//*/

            #region Application Clean Up

            CleanUpApplication(); // Gracefully clean up application prior to close

            #endregion

        }

        /// <summary>
        /// Gets and Creates individual item grammars from text files
        /// </summary>
        static int FindCommands(string[] Location, string Type, string Morph, int k = 0)
        {
            /*//debug1//*/
            try
            {
                /*//debug2//*/
                int i = 0;
                string CurrentID = "";
                string[] nameInfo;
                string ItemName = "";
                string From = "";
                string CurrentCommand = "";
            List<string> CommandList = new List<string>();
                Morph = Morph.ToLower();
                Type = Type.ToLower();
                Choices Commands = new Choices();
                Grammar grammar = new Grammar(new Choices("NA"));
                bool isCommand = false;
                Hashtable table;

                switch (Morph)
                {
                    case "vampirelord":
                        table = AllVampireLordIDs;
                        break;

                    case "werewolf":
                        table = AllWerewolfIDs;
                        break;

                    default:
                        table = AllIDs;
                        break;
                }

            foreach (string currentLocation in Location)
            {

                if (Type != "console")
                {
                    foreach (string commandBlock in File.ReadAllText(currentLocation).ToString().Replace("\r", "").ToLower().Split('['))
                    {
                        CurrentCommand = commandBlock.Split(']')[0];

                        if (CurrentCommand == "" || CurrentCommand == "\n" || CurrentCommand == "\n\n")
                            continue;

                        CommandList = new List<string>();
                        Commands = new Choices();
                        isCommand = false;

                        if (Type == "power") { };

                        foreach (string command in commandBlock.Split(']')[1].Split('\n'))
                        {
                            if (command == "")
                                continue;

                            //Detects if the item is meant to be a command or a general setting for commands 
                            switch (CurrentCommand)
                            {
                                case "hand - left": handLeft.Add(command); isCommand = false; break;
                                case "hand - right": handRight.Add(command); isCommand = false; break;
                                case "hand - both": handBoth.Add(command); isCommand = false; break;
                                case "hand - cast": handCast.Add(command); isCommand = false; break;
                                case "power - cast": powerCast.Add(command); isCommand = false; break;
                                case "location prefixes": locationCommands.Add(command); isCommand = false; break;
                                case "current location commands": currentLocationCommands.Add(command); isCommand = false; break;
                                default: isCommand = true; CommandList.Add(command); break;
                            }
                        }//End Foreach

                        //See why all of the commands for the "hand - left" and similar command types are getting added to AllItems. They should NOT be getting added to that
                        if (!isCommand)
                            continue;

                        grammar = CreateGrammar(Type + "\t" + CurrentCommand, Morph, table, CommandList);
                    }//End for
                }
                else
                {
                    foreach (string line in System.IO.File.ReadAllLines(currentLocation))
                    {
                        //Clean the line
                        string text = line.ToString().Replace("\r", "").Replace("\t", " ").TrimStart(' ').ToLower();

                        //Remove comments
                        if (text.Contains("#"))
                            text = text.Remove(text.IndexOf('#'), text.Length);

                        if (text.Contains("["))
                            text = text.Remove(text.IndexOf('['), text.Length);

                        //If the line is empty, go to the next line
                        if (text == "")
                            continue;

                        //[0] = Voice Commands.   [1] = console commands
                        string[] command = text.Split('=');


                        Commands = new Choices(command[0].Replace("(", "").Trim(')', ' ').Split(')'));

                        CommandList = command[0].Replace("(", "").Trim(')', ' ').Split(')').ToList<string>();
                        grammar = CreateGrammar("console\t" + command[1], Morph, table, CommandList);

                    }
                }
            }//End for

            /*//debug1//*/
        }
        catch (Exception EX) { Log.Activity("Error in \"FindCommands\":\n" + EX.ToString() + "\n", Log.LogType.Error); }
        /*//debug2//*/
            return k;
        }//End FindCommands

        /// <summary>
        /// Gets and Creates progression grammars from text files. Also Creates the individual item grammars if they don't exist
        /// </summary>
        static int FindProgression(string[] Location, string morph, int m)
        {
            try
            {
                //Check what all "AllItems" is supposed to contain. See if changing the name of it would be a good option. Something more descriptive
                //Check in-game to see if progressions works
                //If it does work, and no AllItems changes need to be made, push the changes to Github and look at the to-do list to see what's next. I may need to test Exergist's stuff

                int i = 0,
                    j = 0;
                string CurrentCommand = "";
                List<string> progressionList = new List<string>();
                Choices Commands;
                Grammar grammar;
                Hashtable table = new Hashtable();
                string[] commands = new string[1];
                string fileContents;

                switch (morph)
                {
                    case "vampirelord":
                        table = AllVampireLordIDs;
                        break;

                    case "werewolf":
                        table = AllWerewolfIDs;
                        break;

                    default:
                        table = AllIDs;
                        break;
                }

                //Cycle Through the files in the given location
                foreach (string File in Location)
                {
                    //Get contents of the 
                    fileContents = System.IO.File.ReadAllText(File).ToString().ToLower().Replace("\r", "");

                    string[] allItems = fileContents.ToLower().Split('[');

                    //Cycle Through the progressions in the current file
                    foreach (string itemBlock in allItems)
                    {
                        if (itemBlock == "")
                            continue;

                        //This is used to store the final progression of items
                        progressionList = new List<string>();

                        //Cycle Through the items in the current progression
                        foreach (string item in itemBlock.Split('\n'))
                        {

                            //Save the command
                            if (item.EndsWith("]"))
                            {
                                CurrentCommand = item.Remove(item.Length - 1);
                                Log.AllCommands(CurrentCommand + " - PROGRESSION");
                                continue;

                            }
                            else if (item == "")
                            {
                                continue;
                            }

                            //Create Grammar for the item (this fails if the grammar already exists)
                            grammar = CreateGrammar(item.Split('\t')[2] + '\t' + item, morph, table, new List<string>() { item.Split('\t')[0] });

                            //Add item to the current progression
                            progressionList.Add(grammar.Name);

                        }//Individual Item under a command

                        Commands = new Choices();

                        //If there were no items in the progression, move on to the next file
                        if (progressionList.Count == 0)
                            continue;

                        //Invert the progression to be Worst --> Best
                        progressionList.Reverse();

                        grammar = new Grammar(CreateCommands(progressionList[0], new List<string>() { CurrentCommand }));
                        grammar.Name = CurrentCommand;
                        commands[0] = CurrentCommand;

                        //Add grammar to hashtable
                        if (!AllProgressions.Contains(CurrentCommand))
                            AllProgressions.Add(CurrentCommand, progressionList);

                        //Add each item in the progression to a separate hashtable
                        foreach (string item in progressionList)
                        {

                            if (!AllProgressionItems.Contains(item))
                                AllProgressionItems.Add(item, grammar);
                            else
                                Log.Activity("A duplicate progression item was found: \"" + item + "\"");
                        }

                    }//End Groups of items under a command
                }//End for
            }
            catch (Exception EX) { Log.Activity("Error in \"FindProgression\":\n" + EX.ToString(), Log.LogType.Error); }

            return m;
        }//End FindProgression

        /// <summary>
        /// Returns the best spell associated with a given progression phrase
        /// </summary>
        static Grammar GetProgression(string Phrase, string Morph)
        {
            try
            {
                //Test this to see if it works
                //It looks like this function is looking for items in "AllIDs". Is this meant to look for items in a given morph's table?
                //If so, you need to change that. I think this is the case, so get on that

                Morph = Morph.ToLower();
                List<string> progression = (List<string>)AllProgressions[Phrase.ToLower()];

                if (progression != null)
                {
                    foreach (string item in progression)
                    {
                        if (item == null)
                            continue;

                        if (recognizer.Grammars.Contains((Grammar)AllIDs[item]))
                            return (Grammar)AllIDs[item];
                    }
                }
            }
            catch (Exception EX) { Log.Activity("Error in \"GetProgression\":\n" + EX.ToString(), Log.LogType.Error); }

            //If no loaded grammar is found that mathces the list:
            return new Grammar(new Choices("Not Found"));
        }//End GetProgression

        static int[] DecideGrammar(string type)
        {
            //Log.Debug($"Deciding Grammar for {type}");

            int i, j, k = 0;
            int[] returnVal = { 0, 0 }; //[0] = # items added.  [1] = # items removed
            string[] info;
            string[] newList;
            string[] oldList;
            string newItem = "";
            string oldItem = "";
            Grammar[] progressions = new Grammar[1000];    //Stores progressions to be added, just in case they were removed
            Hashtable table;

            //DateTime start;
            //DateTime end;

            switch (type)
            {

                case "power":
                    newList = KnownPowers;
                    oldList = CurrentPowers;
                    break;

                case "shout":
                    newList = KnownShouts;
                    oldList = CurrentShouts;
                    break;

                default:
                    newList = KnownSpells;
                    oldList = CurrentSpells;
                    break;
            }

            switch (Morph)
            {
                case "werewolf": table = AllWerewolfIDs; break;

                case "vampirelord": table = AllVampireLordIDs; break;

                default: table = AllIDs; break;
            }

            if (newList != oldList)
            {
                //Disable recognition if grammars need to be unloaded. Does not account for a net zero or net positive of added/removed commands, but this rare, so it's fine
                if (newList.Length < oldList.Length || CurrentMorph != Morph)
                {
                    //Log.Debug("recognizer disabled");
                    recognizer.RecognizeAsyncStop();
                }

                if (CurrentMorph != Morph)
                {

                    if (oldList[0] != null)
                    {
                        Log.Debug("Unloading all commands, due to morph change");
                        recognizer.UnloadAllGrammars();

                        oldList = new string[1] { null };
                        CurrentPowers = new string[1] { null };
                        CurrentShouts = new string[1] { null };
                        CurrentSpells = new string[1] { null };
                    }
                }

                for (i = 0, j = 0; i < newList.Length || j < oldList.Length; i++, j++)
                {
                    //start = DateTime.Now;
                    if (i < newList.Length && newList[i] == "")
                    {
                        j--;
                        continue;
                    }

                    //if (i < newList.Length)
                    //    Log.Debug("New" + type + "\t" + newList[i] + '\t' + (i + 1) + "/" + newList.Length + '\t' + (j + 1) + "/" + oldList.Length);
                    //
                    //else if (j < oldList.Length)
                    //    Log.Debug("Old" + type + "\t" + oldList[j] + '\t' + (i + 1) + "/" + newList.Length + '\t' + (j + 1) + "/" + oldList.Length);


                    switch (type)
                    {
                        case "shout":
                            if (i < newList.Length && newList[0] != "")
                            {
                                info = newList[i].Split('\t');
                                newItem = info[0] + '\t' + info[1] + '\t' + info[2] + '\t' + info[3];
                            }

                            if (j < oldList.Length && oldList[0] != null && oldList[0] != "")
                            {
                                info = oldList[j].Split('\t');
                                oldItem = info[0] + '\t' + info[1] + '\t' + info[2] + '\t' + info[3];
                            }
                            break;

                        default:    //Spells and Powers
                            if (i < newList.Length)
                                newItem = newList[i];

                            if (j < oldList.Length)
                                oldItem = oldList[j];
                            break;
                    }


                    if (i < newList.Length && j < oldList.Length && newItem == oldItem && recognizer.Grammars.Contains((Grammar)table[newItem]))
                    {
                        //If they are the same and the new item is already enabled, do nothing.     (the same item from different tables are technically different grammars)

                        if (AllProgressionItems.Contains(newItem))
                        {
                            //Log.EnabledCommands(((Grammar)AllProgressionItems[newItem]).Name + '\t' + "PROGRESSION");
                            progressions[k] = (Grammar)AllProgressionItems[newItem];
                            k++;
                        }

                    }
                    else if (i < newList.Length && !recognizer.Grammars.Contains((Grammar)table[newItem]))
                    {
                        //Spell Added
                        //if they are not the same, it's not a loaded grammar, and it sucessfully created a grammar, then mark it as loaded.

                        if (LoadCommand(newList[i], table) > 0)
                            returnVal[0]++;
                        else
                        {
                            if (CurrentMorph != Morph)
                                returnVal[1]++;
                            //Log.Debug($"Item not added: {newItem}");
                        }

                    }
                    else if (j < oldList.Length && oldItem != null && oldItem != "")
                    {
                        //Item Removed
                        //They are not the same, but it's in a grammar, so it is a removed spell

                        //Log.Debug("Removing: " + ((Grammar)table[oldItem]).Name)

                        if (table.Contains(oldItem))
                            recognizer.UnloadGrammar((Grammar)table[oldItem]);
                        else
                            Log.Debug($"Could not find \"{oldItem}\" in the table associated with Morph \"{Morph}\"", Log.LogType.Error);

                        returnVal[1]++;

                        if (AllProgressionItems.Contains(oldItem) && recognizer.Grammars.Contains((Grammar)AllProgressionItems[oldItem]))
                        {
                            //Log.Debug("Removing Progression: " + ((Grammar)AllProgressionItems[oldItem]).Name);
                            recognizer.UnloadGrammar((Grammar)AllProgressionItems[oldItem]);
                        }
                        i--;

                    }//End if/else

                    //end = DateTime.Now;

                    //Log.Debug($"Letency: {(end - start).Milliseconds}");
                }//End for

                //Add progressions that may have been removed
                foreach (Grammar grammar in progressions)
                {
                    if (grammar == null)
                        break;

                    if (!recognizer.Grammars.Contains(grammar))
                        recognizer.LoadGrammar(grammar);
                }

                switch (type)
                {
                    case "spell":
                        CurrentSpells = newList;
                        break;

                    case "power":
                        CurrentPowers = newList;
                        break;

                    case "shout":
                        CurrentShouts = newList;
                        break;
                }
            }//End lists are not the same
            Log.Debug($"Finished Deciding Grammar for {type}");
            return returnVal;
        }

        /// <summary>
        /// Updates the Voice Recognition's list of grammars
        /// </summary>
        static int ChangeCommands(string Morph)
        {
            /*//debug1//*/
            try
            {
                /*//debug2//*/
                //int currentUpdateNum = updateNum;
                int i = 0, j = 0;
                double duration = 0;
                int ItemsRemoved = 0,
                    ItemsAdded = 0;
                int[] items;


                DateTime MethodStarted = DateTime.Now;

                Hashtable table;

                switch (Morph)
                {
                    case "vampirelord":
                        table = AllVampireLordIDs;
                        break;

                    default:
                        table = AllIDs;
                        break;

                }

                //Clear Enabled Commands File
                Log.EnabledCommands("", Log.LogType.Info, true);

                //Unload all Grammars and FullDictionary
                //recognizer.RecognizeAsyncStop();    //Unloading grammars takes significnatly more time without this line

                //Configure input to the speech recognizer. This is needed for when a new default microphone is set while the program is running, such as if a microphone is disconected then reconnected
                //recognizer.SetInputToDefaultAudioDevice();

                //if (currentUpdateNum != updateNum)
                //    return 0;

                if (!recognizer.Grammars.Contains(FullDictation))
                    recognizer.LoadGrammar(FullDictation);
                ///recognizer.LoadGrammar(FullDictation); /// *** https://learn.microsoft.com/en-us/dotnet/api/system.speech.recognition.speechrecognitionengine.loadgrammarcompleted?view=netframework-4.8.1
                /// RequestRecognizerUpdate ==> https://learn.microsoft.com/en-us/dotnet/api/system.speech.recognition.speechrecognitionengine.requestrecognizerupdate?view=netframework-4.8.1

                //UnloadAllGrammarsDuration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);

                //This is used for testing without running websocket. Allows for testing without running C# through C++
                //KnownSpells = new string[3];
                //KnownSpells[0] = "flames\t12fcd\tspell\tskyrim.esm";
                //KnownSpells[1] = "healing\t12fcc\tspell\tskyrim.esm";
                //KnownSpells[2] = "incinerate\t10f7ed\tspell\tskyrim.esm";

                //LoadCommand(KnownSpells[0], table);

                //CurrentSpells[0] = KnownSpells[0];
                if (Morph != "werewolf")
                {

                    //----------SPELLS----------//
                    if (KnownSpells != null)
                    {

                        items = DecideGrammar("spell");
                        ItemsAdded += items[0];
                        ItemsRemoved += items[1];

                        /*
                            foreach (string item in KnownSpells)
                            {
                                if (item == null || item == "" || currentUpdateNum != updateNum)
                                    break;

                                for (i = 0; i < KnownSpells.Length; i++)
                                {
                                    if (!recognizer.Grammars.Contains((Grammar)table[item]))
                                    {

                                        returned = LoadCommand(item, table);

                                        switch (returned)
                                        {
                                            case 1:
                                                ItemsAdded++;
                                                break;

                                            case -1:
                                                ItemsNotAdded++;
                                                break;
                                        }
                                    }


                                }



                                i++;
                            }//End Spells

                        */
                    }

                    duration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);

                    //if (currentUpdateNum != updateNum)
                    //    return 0;

                    //----------Shouts----------//
                    //KnownShouts = new string[2];
                    //KnownShouts[0] = "Aura Whisper\t7097B\tshout\tSkyrim.esm\tLaas\tYah\tNir";
                    //KnownShouts[1] = "Become Ethereal\t32920\tshout\tSkyrim.esm\tFeim\tZii\tGron";

                    if (KnownShouts != null)
                    {

                        items = DecideGrammar("shout");
                        ItemsAdded += items[0];
                        ItemsRemoved += items[1];



                        /*    foreach (string item in KnownShouts)
                            {
                                if (item == null || item == "" || currentUpdateNum != updateNum)
                                    break;


                                returned = LoadCommand(item, table);

                                switch (returned)
                                {
                                    case 1:
                                        ItemsAdded++;
                                        break;

                                    case -1:
                                        ItemsNotAdded++;
                                        break;
                                }

                           }//End Shouts*/
                    }

                    duration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);

                    //if (currentUpdateNum != updateNum)
                    //    return 0;

                    //----------POWERS----------//
                    if (KnownPowers != null)
                    {
                        items = DecideGrammar("power");
                        ItemsAdded += items[0];
                        ItemsRemoved += items[1];

                        /*
                                foreach (string item in KnownPowers)
                                {
                                    if (item == null || item == "" || currentUpdateNum != updateNum)
                                        break;

                                    returned = LoadCommand(item, table);

                                    switch (returned)
                                    {
                                        case 1:
                                            ItemsAdded++;
                                            break;

                                        case -1:
                                            ItemsNotAdded++;
                                            break;
                                    }

                                }//End Powers
                                */

                    }

                    duration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);

                    //if (currentUpdateNum != updateNum)
                    //    return 0;
                }//End Werewolf morph check       

                if (CurrentMorph != Morph)
                {
                    if (Morph == "werewolf")
                    {
                        Log.Debug("Unloading all commands, due to morph change");
                        recognizer.RecognizeAsyncStop();
                        recognizer.UnloadAllGrammars(); //If the player just turned into a werewolf, unload all commands
                    }

                    //Add Keybinds
                    if (KeybindGrammars != null)
                    {
                        //Add New Grammars
                        foreach (Grammar grammar in KeybindGrammars)
                        {
                            if (grammar == null)
                                break;

                            //Valid Keybinds
                            if (grammar.Name.StartsWith(Morph.ToLower()) || Morph.ToLower() == "dragonriding" && grammar.Name.StartsWith("none") || Morph.ToLower() == "horseriding" && grammar.Name.StartsWith("none"))
                            {
                                if (!recognizer.Grammars.Contains(grammar))
                                {
                                    recognizer.LoadGrammar(grammar);
                                    ItemsAdded++;
                                }
                            }
                            else if (recognizer.Grammars.Contains(grammar))
                            {
                                recognizer.LoadGrammar(grammar);
                                ItemsRemoved++;
                            }
                        }//End for each Keybind Grammar
                    }//End Keybind Grammars not NULL

                    //Add Console Commands
                    if (ConsoleGrammars != null)
                    {
                        //Add New Grammars
                        foreach (Grammar grammar in ConsoleGrammars)
                        {
                            if (grammar == null)
                                break;

                            if (!recognizer.Grammars.Contains(grammar))
                                recognizer.LoadGrammar(grammar);

                        }//End foreach Console Grammar
                    }//End Console Grammars not NULL

                    CurrentMorph = Morph;
                }//End Morph Changed

                duration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);

                //Add Voiced Settings
                foreach (Grammar grammar in SettingGrammars)
                {
                    if (grammar == null)
                        break;

                    if (recognizer.Grammars.Contains(grammar))
                        recognizer.UnloadGrammar(grammar);
                }

                foreach (Grammar grammar in SettingGrammars)
                {

                    if (grammar == null)
                        break;

                    if ((VocalPushToSpeak || !VocalPushToSpeak &&
                        grammar.Name.ToLower() != "vocal command toggle - enable\tsetting" &&
                        grammar.Name.ToLower() != "vocal command toggle - disable\tsetting") &&
                        (Morph != "werewolf" || grammar.Name != "clear shout\tsetting") &&
                        (Morph != "vampirelord" || grammar.Name != "clear hands\tsetting"))
                    {
                        recognizer.LoadGrammar(grammar);
                    }
                }

                duration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);

                //if (currentUpdateNum != updateNum)
                //    return 0;

                try
                {
                    recognizer.RecognizeAsync(RecognizeMode.Multiple);
                }
                catch (Exception) { } //No updated to Voiced Settings

                foreach (Grammar grammar in recognizer.Grammars)
                {
                    if (grammar.Name == "FullDictation")
                        continue;

                    if (grammar.Name.Split('\t').Length == 1)
                    {
                        Log.EnabledCommands(grammar.Name + " - PROGRESSION");
                    }
                    else
                        Log.EnabledCommands(grammar.Name);
                }

                //ChangeCommands latency
                duration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);
                Log.Activity($"Updating Finished - {DateTime.Now.ToLongTimeString()} (Elapsed Time: {duration}s.) ({ItemsAdded} items added. {ItemsRemoved} items removed)\n");

                /*
                duration = Math.Round((DateTime.Now - MethodStarted).TotalSeconds, 2);
                Log.Activity("Updating Finished - " + DateTime.Now.ToLongTimeString() + " (Elapsed Time: " + duration + "s. " +
                    Math.Round((duration - UnloadAllGrammarsDuration) / (ItemsAdded + ItemsNotAdded), 4) + " seconds per item. " +
                    ItemsAdded + " out of " + (ItemsAdded + ItemsNotAdded) + " added. Unloading commands took " + UnloadAllGrammarsDuration + "s)\n");
                    */
                //Sort EnabledCommands List
                string[] lines = System.IO.File.ReadAllLines(Log.logs.EnabledCommands);
                Array.Sort(lines);
                File.WriteAllLines(Log.logs.EnabledCommands, lines);

                /*//debug1//*/
            }
            catch (Exception ex)
            {
                Log.Activity("Error in \"ChangeCommands\": " + ex.ToString() + "\n", Log.LogType.Error);
            }
            /*//debug2//*/

            return 1;
        }//End ChangeCommands

        /// <summary>
        /// Loads a given item's grammar into Speech Recognition. Creates the grammar if it doesn't exist
        /// </summary>
        static int LoadCommand(string item, Hashtable table)
        {
            /*//debug1//*/
            try
            {
                /*//debug2//*/
                if (item == null || item == "")
                    return 0;

                string[] info = item.ToLower().Split('\t');
                string name = info[0];
                string id = info[1];
                string type = info[2];
                string from = info[3];
                string current;

                if (info[1].Length > 6)
                {
                    id = info[1].Remove(0, info[1].Length - 6);
                }

                id = id.TrimStart('0', '0', '0', '0', '0', '0');

                current = name + '\t' + id + '\t' + type + '\t' + from;

                if (table.Contains(current + "\tignore"))
                    current = current + "\tignore";

                else if (table.Contains(current + "\tleft"))
                    current = current + "\tleft";

                else if (table.Contains(current + "\tright"))
                    current = current + "\tright";

                else if (table.Contains(current + "\tboth"))
                    current = current + "\tboth";

                if (table.Contains(current) && !recognizer.Grammars.Contains((Grammar)table[current]))
                {
                    recognizer.LoadGrammar((Grammar)table[current]);
                    //Log.EnabledCommands(((Grammar)table[current]).Name);

                    if (AllProgressionItems.Contains(current) && !recognizer.Grammars.Contains((Grammar)AllProgressionItems[current]))
                    {
                        recognizer.LoadGrammar((Grammar)AllProgressionItems[current]);
                        //Log.EnabledCommands(((Grammar)AllProgressionItems[current]).Name + '\t' + "PROGRESSION");
                    }

                    return 1;

                    //Spell is NOT an existing garmmar
                }
                else if (!table.Contains(current))
                {
                    if (Morph != "vampirelord")
                    {
                        List<string> commandList = new List<string>();

                    switch (type)
                    {
                        case "shout":
                            commandList.Add(info[4]);
                            commandList.Add($"{info[4]} {info[5]}");
                            commandList.Add($"{info[4]} {info[5]} {info[6]}");
                            break;

                        default:
                            commandList.Add(name);
                            break;
                    }

                    recognizer.LoadGrammar(CreateGrammar(type + '\t' + current, "none", table, commandList));
                        //Log.EnabledCommands(current);

                        return 1;
                    }

                }
                else if (Morph.ToLower() == "dragonriding" && table.Contains(current) && !recognizer.Grammars.Contains((Grammar)table[current]))
                {

                    recognizer.LoadGrammar((Grammar)table[current]);
                    //Log.EnabledCommands(((Grammar)table[current]).Name);

                    return 1;

                    //If the grammar is already loaded, then that means the item is in multiple progressions. This is fine
                }
                else if (AllProgressionItems.Contains(Morph.ToLower() + "\t" + current) && !recognizer.Grammars.Contains((Grammar)AllProgressionItems[Morph.ToLower() + "\t" + current]))
                {
                    recognizer.LoadGrammar((Grammar)AllProgressionItems[Morph.ToLower() + " - " + current]);
                }

                /*//debug1//*/
            }
            catch (Exception ex)
            {
                Log.Activity("Error in \"LoadCommand\": " + ex.ToString() + "\n", Log.LogType.Error);
            }
            /*//debug2//*/


            return -1;
        }//End Load Command

        /// <summary>
        /// Creates a grammar for a given item with the given array of commands
        /// </summary>
        static Grammar CreateGrammar(string item, string morph, Hashtable table, List<string> commandList)
        {
            /*//debug1//*/
            try
            {
                /*//debug2//*/
            Choices Commands = new Choices();
            Grammar grammar = new Grammar(new Choices("NA"));

            string[] info = item.Split('\t');   //[0] == name   [1] == ID   [2] == from
            string name = info[1];
            string id = "";
            string type = info[0];
            string from = "";
            string mod = "";

            item = name;

            switch (type)
            {
                case "keybind":
                    id = info[2];
                    item += '\t' + id;
                    break;

                case "setting": break;

                case "console": break;
                    

                default:
                    id = '\t' + info[2].TrimStart('0', '0', '0', '0', '0', '0'); ;
                    from = '\t' + info[4];

                    if (info.Length >= 6)
                        mod = '\t' + info[5];  //Moddifications suchs as specified hand and autocast

                    item += id + '\t' + type + from + mod;
                    break;

            }


            //Typo Checker for files
            switch (type)
            {
                case "spells": Log.Activity($"The item \"{name}\" from \"{from}\" has an invalid type. It needs to be \"Spell\". not \"Spells\""); break;
                case "powers": Log.Activity($"The item \"{name}\" from \"{from}\" has an invalid type. It needs to be \"Shout\". not \"Shouts\""); break;
                case "shouts": Log.Activity($"The item \"{name}\" from \"{from}\" has an invalid type. It needs to be \"Power\". not \"Powers\""); break;
            }


            grammar = new Grammar(CreateCommands(item, commandList));

            switch (type)
            {
                case "keybind":
                    grammar.Name = morph + "\t" + name + '\t' + id + '\t' + type;

                    break;

                case "console":
                case "setting":
                    grammar.Name = name + '\t' + type;
                    break;

                default:
                    grammar.Name = item;
                    break;
            }


            if (!table.Contains(grammar.Name) && !table.Contains(name + '\t' + id + '\t' + type + '\t' + from))
            {
                switch (type)
                {
                    case "keybind":
                        KeybindGrammars.Add(grammar);
                        break;

                    case "console":
                        ConsoleGrammars.Add(grammar);
                        break;

                    case "setting":
                        SettingGrammars.Add(grammar);
                        break;

                    default:
                        table.Add(grammar.Name, grammar);
                        break;
                }

                foreach (string command in commandList)
                {
                    Log.AllCommands(command + " - " + grammar.Name);

                    if (!AllItems.Contains(morph + " - " + command))
                    {
                        AllItems.Add(morph + " - " + command, grammar);
                    }
                    else
                    {
                        //If the item is from the same mod (has the same "from" value), output an error instead of adding it to the progression
                        if (((Grammar)AllItems[Morph + " - " + command]).Name == grammar.Name)
                        {
                            Log.Activity($"Two or more identical items exist from the same mod. You must change the commands for one of them: \"{name}\t{id}\t{type}\t{from}\"", Log.LogType.Error);

                        }
                        else if (AllProgressions.Contains(command))
                        {
                            List<string> newOptions = new List<string>();
                            newOptions =  (List<string>)AllProgressions[command];
                            newOptions.Add(grammar.Name);
                            AllProgressions[command] = newOptions;
                        }
                        else
                        {
                            AllProgressions.Add(command, new List<string>() { ((Grammar)AllItems[Morph + " - " + command]).Name, grammar.Name});
                            Log.Activity($"Two or more identical items have the same command for the same morph ({morph}): \"{command}\". Item 1: \"{((Grammar)AllItems[Morph + " - " + command]).Name}\". Item 2: \"{grammar.Name}\"", Log.LogType.Error);
                        }//End if
                    }//End Try/Catch


                    //AllItems here
                }
            }
            else
                grammar = (Grammar)table[grammar.Name];

            return grammar;
            /*//debug1//*/
        }
        catch (Exception EX)
        {
            Log.Activity("Error in \"CreateGrammar\":\n" + EX.ToString() + "\n", Log.LogType.Error);
            return new Grammar(new Choices("NA"));
        }
        /*//debug2//*/

        }//End CreateGrammar

        /// <summary>
        /// Returns the modified commands associated with a given item and given list of initial commands
        /// </summary>
        static Choices CreateCommands(string title, List<string> items)
        {
            Choices commands = new Choices();
            string command;

            string[] info = title.Split('\t');
            string name = info[0];
            string type = "";

            if (info.Length >= 3)
                type = info[2];

            //Fill in item's title
            foreach (string item in items)
            {
                //Correct Dovahzul Pronunciations

                foreach (string dovahzulWord in item.Split(' '))
                {
                    string phonemes = dovahzulPhonemes.GetString(dovahzulWord);
                    if (phonemes != null && !trainedWords.Contains(dovahzulWord))
                    {
                        //Exergist, this is your section
                        //Dovahzul: item
                        //Phoneems: phonemes
                        //Using the above information, add to the Speech Dictionary
                        trainedWords.Add(dovahzulWord);
                    }
                }
                command = item;

                //Add the raw command ("Conjure Familiar")
                commands.Add(command);

                //Command specifications
                switch (type.ToLower())
                {
                    case "spell":
                        bool repeat = false;

                        bool isConjure = false;
                        bool isSummon = false;
                        string originalName = command;

                        do
                        {
                            repeat = false;

                            //Left Hand ("Left Conjure Familiar")
                            foreach (string a in handLeft)
                            {
                                commands.Add(command + "\t" + a);
                                commands.Add(a + "\t" + command);

                                //Hand Cast
                                foreach (string b in handCast)
                                {
                                    commands.Add(b + "\t" + a + "\t" + command);
                                    commands.Add(b + "\t" + command + "\t" + a);
                                    commands.Add(a + "\t" + b + "\t" + command);
                                }


                            }

                            //Hand Right ("Right Conjure Familiar")
                            foreach (string a in handRight)
                            {
                                commands.Add(command + "\t" + a);
                                commands.Add(a + "\t" + command);

                                //Hand Cast
                                foreach (string b in handCast)
                                {
                                    commands.Add(b + "\t" + a + "\t" + command);
                                    commands.Add(b + "\t" + command + "\t" + a);
                                    commands.Add(a + "\t" + b + "\t" + command);

                                }

                            }


                            //Hand Both ("Both Conjure Familiar")
                            foreach (string a in handBoth)
                            {
                                commands.Add(command + "\t" + a);
                                commands.Add(a + "\t" + command);

                                //Hand Cast
                                foreach (string b in handCast)
                                {
                                    commands.Add(b + "\t" + a + "\t" + command);
                                    commands.Add(b + "\t" + command + "\t" + a);
                                    commands.Add(a + "\t" + b + "\t" + command);
                                }

                            }

                            //Hand Cast ("Cast Conjure Familiar")
                            foreach (string a in handCast)
                            {
                                commands.Add(a + "\t" + command);

                            }

                            //Additional commands with removed prefixes for more fluid commands ("Conjure Familiar" --> "Familiar" for all above command variations)
                            if (!isConjure && originalName.StartsWith("conjure"))
                            {
                                command = originalName.Remove(0, 8);
                                isConjure = true;
                                repeat = true;

                            }
                            else if (!isSummon && originalName.StartsWith("summon"))
                            {
                                command = originalName.Remove(0, 7);
                                isSummon = true;
                                repeat = true;
                            }

                            //Add new command ("Familiar")
                            if (repeat)
                                commands.Add(command);

                        } while (repeat);


                        break;

                    case "power":
                        foreach (string a in powerCast)
                        {
                            commands.Add(a + "\t" + command);
                        }

                        break;
                }//End switch
            }//End Cycle Items


            return commands;
        }

        /// <summary>
        /// Turns a string key ("W", "LShift") into a Skyrim Key Code
        /// </summary>
        static int ToSkyrimKeyCode(string KeyString)
        {
            int Key = 0;

            try
            {
                //https://www.creationkit.com/index.php?title=Input_Script

                switch (KeyString.ToUpper())
                {
                    case "NONE": Key = 0; break;
                    case "ESCAPE": Key = 1; break;
                    case "1": Key = 2; break;
                    case "2": Key = 3; break;
                    case "3": Key = 4; break;
                    case "4": Key = 5; break;
                    case "5": Key = 6; break;
                    case "6": Key = 7; break;
                    case "7": Key = 8; break;
                    case "8": Key = 9; break;
                    case "9": Key = 10; break;
                    case "0": Key = 11; break;
                    case "-": Key = 12; break;
                    case "=": Key = 13; break;
                    case "BACKSPACE": Key = 14; break;
                    case "\\t":
                    case "TAB": Key = 15; break;
                    case "Q": Key = 16; break;
                    case "W": Key = 17; break;
                    case "E": Key = 18; break;
                    case "R": Key = 19; break;
                    case "T": Key = 20; break;
                    case "Y": Key = 21; break;
                    case "U": Key = 22; break;
                    case "I": Key = 23; break;
                    case "O": Key = 24; break;
                    case "P": Key = 25; break;
                    case "[": Key = 26; break;
                    case "]": Key = 27; break;
                    case "ENTER": Key = 28; break;
                    case "LCTRL": Key = 29; break;
                    case "A": Key = 30; break;
                    case "S": Key = 31; break;
                    case "D": Key = 32; break;
                    case "F": Key = 33; break;
                    case "G": Key = 34; break;
                    case "H": Key = 35; break;
                    case "J": Key = 36; break;
                    case "K": Key = 37; break;
                    case "L": Key = 38; break;
                    case ";": Key = 39; break;
                    case "\"": Key = 40; break;
                    case "~": Key = 41; break;
                    case "LSHIFT": Key = 42; break;
                    case "\\": Key = 43; break;
                    case "Z": Key = 44; break;
                    case "X": Key = 45; break;
                    case "C": Key = 46; break;
                    case "V": Key = 47; break;
                    case "B": Key = 48; break;
                    case "N": Key = 49; break;
                    case "M": Key = 50; break;
                    case ",": Key = 51; break;
                    case ".": Key = 52; break;
                    case "/": Key = 53; break;
                    case "RSHIFT": Key = 54; break;
                    case "LALT": Key = 56; break;
                    case "SPACE": Key = 57; break;
                    case "CAPS": Key = 58; break;
                    case "F1": Key = 59; break;
                    case "F2": Key = 60; break;
                    case "F3": Key = 61; break;
                    case "F4": Key = 62; break;
                    case "F5": Key = 63; break;
                    case "F6": Key = 64; break;
                    case "F7": Key = 65; break;
                    case "F8": Key = 66; break;
                    case "F9": Key = 67; break;
                    case "F10": Key = 68; break;
                    case "F11": Key = 87; break;
                    case "F12": Key = 88; break;
                    case "RCTRL": Key = 157; break;
                    case "RALT": Key = 184; break;
                    case "UP": Key = 200; break;
                    case "LEFT": Key = 203; break;
                    case "RIGHT": Key = 205; break;
                    case "END": Key = 207; break;
                    case "DOWN": Key = 208; break;
                    case "DELETE": Key = 211; break;
                    case "MOUSE1": Key = 256; break;
                    case "MOUSE2": Key = 257; break;
                    case "MIDDLEMOUSE": Key = 258; break;
                    case "MOUSE3": Key = 259; break;
                    case "MOUSE4": Key = 260; break;
                    case "MOUSE5": Key = 261; break;
                    case "MOUSE6": Key = 262; break;
                    case "MOUSE7": Key = 263; break;
                    case "MOUSEWHEELUP": Key = 264; break;
                    case "MOUSEWHEELDOWN": Key = 265; break;
                    case "GAMEPAD DPAD_UP": Key = 266; break;
                    case "GAMEPAD DPAD_DOWN": Key = 267; break;
                    case "GAMEPAD DPAD_LEFT": Key = 268; break;
                    case "GAMEPAD DPAD_RIGHT": Key = 269; break;
                    case "GAMEPAD START": Key = 270; break;
                    case "GAMEPAD BACK": Key = 271; break;
                    case "GAMEPAD LEFT_THUMB": Key = 272; break;
                    case "GAMEPAD RIGHT_THUMB": Key = 273; break;
                    case "GAMEPAD LEFT_SHOULDER": Key = 274; break;
                    case "GAMEPAD RIGHT_SHOULDER": Key = 275; break;
                    case "GAMEPAD A": Key = 276; break;
                    case "GAMEPAD B": Key = 277; break;
                    case "GAMEPAD X": Key = 278; break;
                    case "GAMEPAD Y": Key = 279; break;
                    case "GAMEPAD LT": Key = 280; break;
                    case "GAMEPAD RT": Key = 281; break;

                    case "ALL": Key = -1; break;
                }

            }
            catch (Exception EX) { Log.Activity("Error in \"ToSkyrimKeyCode\":\n" + EX.ToString() + "\n", Log.LogType.Error); }

            return Key;
        }

        #region Event Methods

        /// <summary>
        /// Event when a target process is closed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private static void OnProcessClosed(object sender, ProcessWatcher.ProcessClosedEventArgs e)
        {
            Log.Debug($"Target process \"{e.ProcessName}\" closed", Log.LogType.Info);

            ///runApplication = false; // Unset boolean flag indicating that speech recognition application should terminate

            // Stop and dispose the WebSocketServer (if it's not null)
            if (server != null)
            {
                server.OnConnected -= ClientConnected;
                server.OnDisconnected -= ClientDisconnected;
                server.OnMessageReceived -= ClientMessageReceived;
                server?.DisposeServer();
                server = null;
            }

            // Halt application "waiting" and continue executing on the main thread
            try { waitHandle.Set(); }
            finally { waitHandle.Close(); }
        }

        /// <summary>
        /// Executes when websocket server (this app) connects to client (SKSE plugin)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private static void ClientConnected(object sender, EventArgs e)
        {
            Log.Debug($"Application connected to SKSE plugin", Log.LogType.Info);
        }

        /// <summary>
        /// Executes when websocket server (this app) disconnects from client (SKSE plugin)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private static void ClientDisconnected(object sender, EventArgs e)
        {
            Log.Debug($"Application disconnected from SKSE plugin", Log.LogType.Info);
        }

        /// <summary>
        /// Executes when observableConcurrentQueue collection changes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private static void OnObservableConcurrentQueueCollectionChanged(object sender, NotifyCollectionChangedEventArgs args)
        {
            string message;
            try
            {
                switch (args.Action)
                {
                    case NotifyCollectionChangedAction.Add:
                        Log.Debug($"[+] Collection Changed [Add]: New Item added: {args.NewItems[0]}", Log.LogType.Info);
                        while (observableConcurrentQueue.Count > 0)
                        {
                            if (observableConcurrentQueue.TryDequeue(out message) == true)
                                ProcessClientMessage(message);
                            else
                                Log.Debug($"Could not queue message for processing: {message}", Log.LogType.Info);
                        }

                        //foreach (var obsCQ in observableConcurrentQueue)
                        //{
                        //    if (observableConcurrentQueue.TryDequeue(out message) == true)
                        //        ProcessClientMessage(message);
                        //    else
                        //        Log.Debug($"Could not queue message for processing: {message}", Log.LogType.Info);
                        //}
                        //if (observableConcurrentQueue.Count == 0)
                        //{
                        //}
                        //else
                        //{
                        //    foreach (var obsCQ in observableConcurrentQueue)
                        //    {
                        //        if (observableConcurrentQueue.TryDequeue(out message) == true)
                        //            ProcessClientMessage(message);
                        //        else
                        //            Log.Debug($"Could not queue message for processing: {message}", Log.LogType.Info);
                        //    }
                        //}
                        break;
                    case NotifyCollectionChangedAction.Remove:
                        Log.Debug($"[-] Collection Changed [Remove]: New Item deleted: {args.OldItems[0]}", Log.LogType.Info);
                        break;
                    case NotifyCollectionChangedAction.Reset:
                        Log.Debug("[ ] Collection Changed [Reset]: Queue is empty", Log.LogType.Info);
                        break;
                }
            }
            catch (Exception ex)
            {
                Log.Debug($"Error processing message queue collection change: {ex.Message}", Log.LogType.Error);
            }
        }

        /// <summary>
        /// Executes when websocket server (this app) receives a message from the client (SKSE plugin)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private static void ClientMessageReceived(object sender, WebSocketServer.ExMessageReceivedEventArgs e)
        {
            string message = e.Message;
            ProcessClientMessage(message);
        }//End ClientMessageReceived

        /// <summary>
        /// Process messages from client (Skyrim plugin)
        /// </summary>
        private static void ProcessClientMessage(string message)
        {
            if (message.StartsWith("update configuration"))
            {
                Log.Debug($"Received from client: Update Configuration", Log.LogType.Info);
                Morph = "none";

                foreach (string item in message.Remove(0, 21).Replace("\r", "").TrimEnd('\n').ToLower().Split('\n'))
                {
                    switch (item.Split('\t')[0])
                    {
                        case "werewolf":
                        case "vampirelord":
                        case "dragonriding":
                        case "horseriding":
                            if (item.Split('\t')[1].ToLower() == "true")
                                Morph = item.Split('\t')[0];

                            break;

                        case "vox_sensitivity":
                            Sensitivity = float.Parse(item.Split('\t')[1]);
                            FullDictation.Weight = (100 - Sensitivity) / 100;
                            break;

                        case "vox_autocastshouts":
                            AutoCastShouts = Convert.ToBoolean(item.Split('\t')[1]);
                            break;

                        case "vox_autocastpowers":
                            AutoCastPowers = Convert.ToBoolean(item.Split('\t')[1]);
                            break;

                    }// End switch
                }// End ForEach


            }
            else if (message.StartsWith("update spells"))
            {
                Log.Debug($"Received from client: Update Spells", Log.LogType.Info);
                KnownSpells = message.Remove(0, 14).TrimEnd('\n').ToLower().Split('\n');
                //Log.Debug(message.Remove(0, 14));

            }
            else if (message.StartsWith("update shouts"))
            {
                Log.Debug($"Received from client: Update Shouts", Log.LogType.Info);
                KnownShouts = message.Remove(0, 14).TrimEnd('\n').ToLower().Split('\n');
                //Log.Debug(message.Remove(0, 14).TrimEnd('\n').ToLower());

            }
            else if (message.StartsWith("update powers"))
            {
                Log.Debug($"Received from client: Update Powers", Log.LogType.Info);
                KnownPowers = message.Remove(0, 14).TrimEnd('\n').ToLower().Split('\n');

            }
            else if (message.StartsWith("update locations"))
            {
                Log.Debug($"Received from client: Update Locations", Log.LogType.Info);
                KnownLocations = message.Remove(0, 17).TrimEnd('\n').ToLower().Split('\n');
                Grammar grammar;
                Choices commands = new Choices();

                foreach (string location in KnownLocations)
                {
                    if (location == null)
                        break;

                    if (!LocationsGrammars.Contains(location.ToLower()))
                    {
                        commands = new Choices();
                        commands.Add(location.ToLower());

                        foreach (string item in locationCommands)
                        {
                            if (item == null)
                                break;

                            commands.Add(item + " " + location.ToLower());
                        }

                        grammar = new Grammar(commands);
                        grammar.Name = $"{location.ToLower()}\tlocation";
                        LocationsGrammars.Add(location.ToLower(), grammar);
                    }
                }

            }
            else if (message.StartsWith("enable location commands"))
            {
                Log.Debug($"Received from client: Enable Location Commands", Log.LogType.Info);

                foreach (string location in KnownLocations)
                {
                    if (location == null)
                        break;

                    if (LocationsGrammars.Contains(location.ToLower()))
                    {
                        if (!recognizer.Grammars.Contains((Grammar)LocationsGrammars[location.ToLower()]))
                            recognizer.LoadGrammar((Grammar)LocationsGrammars[location.ToLower()]);
                        else
                            Log.Debug($"Multiple locations have the name \"{location.ToLower()}\"", Log.LogType.Error);
                    }
                    else
                    {
                        Choices commands = new Choices();
                        Grammar grammar;
                        commands.Add(location.ToLower());

                        foreach (string item in locationCommands)
                        {
                            if (item == null)
                                break;

                            commands.Add(item + " " + location.ToLower());
                        }

                        grammar = new Grammar(commands);
                        grammar.Name = $"{location.ToLower()}\tlocation";
                        LocationsGrammars.Add(location.ToLower(), grammar);
                    }

                }

            }
            else if (message.StartsWith("disable location commands"))
            {
                Log.Debug($"Received from client: Disable Location Commands", Log.LogType.Info);

                foreach (string location in KnownLocations)
                {
                    if (location == null)
                        break;

                    if (LocationsGrammars.Contains(location.ToLower()))
                    {
                        if (recognizer.Grammars.Contains((Grammar)LocationsGrammars[location.ToLower()]))
                            recognizer.UnloadGrammar((Grammar)LocationsGrammars[location.ToLower()]);
                    }
                }

            }
            else if (message.StartsWith("initialize update"))
            {
                Log.Debug($"Received from client: Initialize Update", Log.LogType.Info);

                updateNum++;
                int currentUpdateNum = updateNum;

                while (isUpdating && currentUpdateNum == updateNum)
                {
                    Task.Delay(10);
                }

                if (!isUpdating)
                {
                    isUpdating = true;
                    Log.Debug($"Initialization Started (Num: {currentUpdateNum})", Log.LogType.Info);
                    if (ChangeCommands(Morph) == 0)
                    {
                        Log.Debug($"Stopped current update in favor of a new incoming update", Log.LogType.Info);
                    }

                    Log.Debug($"Initialization Finished (Num: {currentUpdateNum})\n", Log.LogType.Info);
                    isUpdating = false;

                }
                else
                {
                    Log.Debug($"Bumped queued update {currentUpdateNum} in favor of a newer update", Log.LogType.Info);
                }

            }
            else
            {
                Log.Debug($"Received from client: {message}", Log.LogType.Info);
            }

        }

        /// <summary>
        /// Executes when target websocket port configuration file changes
        /// </summary>
        /// <param name="source"></param>
        /// <param name="e"></param>
        private static void OnPortConfigChanged(object source, FileSystemEventArgs e)
        {
            // Log the type of change to the websocket port configuration file
            Log.Debug("File: " + e.FullPath + " " + e.ChangeType, Log.LogType.Info);

            // Apply the new websocket port from the target file
            string portConfigData = File.ReadAllText(e.FullPath);
            if (Int32.TryParse(portConfigData, out int port) == true && port >= 1050 && port <= 65500)
            {
                websocketPort = port;
                Log.Debug($"Imported websocket port configuration from SKSE plugin: {websocketPort}", Log.LogType.Info);
            }
            else
            {
                Log.Debug($"Imported websocket port ({port}) is not a valid integer", Log.LogType.Error);
                if (websocketPort == defaultWebsocketPort)
                    Log.Debug($"Websocket server will continue leveraging default port (no change)", Log.LogType.Info);
                else
                {
                    Log.Debug($"Websocket server will leverage default port: {defaultWebsocketPort}", Log.LogType.Info);
                    websocketPort = defaultWebsocketPort;
                    StartWebsocketServer(ip, websocketPort);
                }
            }
        }

        /*
         * Event to use for "Sound attracts attention" feature. This event seems to trigger constantly, even when the audio level doesn't change. The if-statement gives the illusion of it not triggering constantly
        static void Recognizer_AudioLevelUpdated(object sender, AudioLevelUpdatedEventArgs e)
        {
            if (e.AudioLevel > 10)
            {
                return;
            }
        }
        */

        /// <summary>
        /// Executes when user speech is recognized (SpeechRecognized event)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        static void Recognizer_SpeechRecognized(object sender, SpeechRecognizedEventArgs e)
        {
            /*//debug1//*/
            try
            {
                /*//debug2//*/

                ///throw new Exception("ex test");
                //End recognition if the program isn't confident about what we said or it's from the FullDictation
                if (e.Result.Confidence < 0.2 || e.Result.Grammar.Name == "FullDictation") return;

                string name = "";
                string id = "";
                string type = "";
                string from = "";
                int hand = -1;  //-1 means it is unset. Leaving it unset may break on the mod's side, so ensure it changes
                bool autoCast = false;
                string autoHand = "";
                string[] info;
                DateTime latency = DateTime.Now;

                int keyDuration = 0;

                string Command = e.Result.Text;
                string Title = "";

                string Text = "";

                ///Log.Debug($"Command = {Command}", Log.LogType.Info);


                //Hand Left
                foreach (string a in handLeft)
                {

                    foreach (string b in handCast)
                    {

                        if (e.Result.Text.StartsWith(a + '\t' + b + '\t') || e.Result.Text.StartsWith(b + '\t' + a + '\t'))
                        {
                            Command = e.Result.Text.Remove(0, a.Length + b.Length + 2);
                            hand = 0;
                            autoCast = true;
                            break;

                        }

                        if (e.Result.Text.StartsWith(b + '\t') && e.Result.Text.EndsWith('\t' + a))
                        {
                            Command = e.Result.Text.Remove(0, b.Length + 1);
                            Command = Command.Remove(Command.Length - a.Length - 1, a.Length + 1);
                            hand = 0;
                            autoCast = true;
                            break;
                        }
                    }

                    if (Command == e.Result.Text)
                    {
                        if (e.Result.Text.StartsWith(a + '\t'))
                        {
                            Command = e.Result.Text.Remove(0, a.Length + 1);
                            hand = 0;
                            break;

                        }
                        else if (e.Result.Text.EndsWith('\t' + a))
                        {
                            Command = e.Result.Text.Remove(e.Result.Text.Length - a.Length - 1, a.Length + 1);
                            hand = 0;
                            break;
                        }
                    }
                }

                //Hand Right
                if (Command == e.Result.Text)
                    foreach (string a in handRight)
                    {

                        foreach (string b in handCast)
                        {

                            if (e.Result.Text.StartsWith(a + '\t' + b + '\t') || e.Result.Text.StartsWith(b + '\t' + a + '\t'))
                            {
                                Command = e.Result.Text.Remove(0, a.Length + b.Length + 2);
                                hand = 1;
                                autoCast = true;
                                break;

                            }

                            if (e.Result.Text.StartsWith(b + '\t') && e.Result.Text.EndsWith('\t' + a))
                            {
                                Command = e.Result.Text.Remove(0, b.Length + 1);
                                Command = Command.Remove(Command.Length - a.Length - 1, a.Length + 1);
                                hand = 1;
                                autoCast = true;
                                break;
                            }
                        }

                        if (Command == e.Result.Text)
                        {
                            if (e.Result.Text.StartsWith(a + '\t'))
                            {
                                Command = e.Result.Text.Remove(0, a.Length + 1);
                                hand = 1;
                                break;

                            }
                            else if (e.Result.Text.EndsWith('\t' + a))
                            {
                                Command = e.Result.Text.Remove(e.Result.Text.Length - a.Length - 1, a.Length + 1);
                                hand = 1;
                                break;
                            }
                        }
                    }

                //Hand Both
                if (Command == e.Result.Text)
                    foreach (string a in handBoth)
                    {

                        foreach (string b in handCast)
                        {

                            if (e.Result.Text.StartsWith(a + '\t' + b + '\t') || e.Result.Text.StartsWith(b + '\t' + a + '\t'))
                            {
                                Command = e.Result.Text.Remove(0, a.Length + b.Length + 2);
                                hand = 2;
                                autoCast = true;
                                break;

                            }

                            if (e.Result.Text.StartsWith(b + '\t') && e.Result.Text.EndsWith('\t' + a))
                            {
                                Command = e.Result.Text.Remove(0, b.Length + 1);
                                Command = Command.Remove(Command.Length - a.Length - 1, a.Length + 1);
                                hand = 2;
                                autoCast = true;
                                break;
                            }
                        }

                        if (Command == e.Result.Text)
                        {
                            if (e.Result.Text.StartsWith(a + '\t'))
                            {
                                Command = e.Result.Text.Remove(0, a.Length + 1);
                                hand = 2;
                                break;

                            }
                            else if (e.Result.Text.EndsWith('\t' + a))
                            {
                                Command = e.Result.Text.Remove(e.Result.Text.Length - a.Length - 1, a.Length + 1);
                                hand = 2;
                                break;
                            }
                        }
                    }

                //Hand Cast
                if (Command == e.Result.Text)
                    foreach (string a in handCast)
                    {
                        if (e.Result.Text.StartsWith(a + '\t'))
                        {
                            Command = e.Result.Text.Remove(0, a.Length + 1);
                            autoCast = true;
                            break;

                        }
                        else if (e.Result.Text.EndsWith('\t' + a))
                        {
                            Command = e.Result.Text.Remove(e.Result.Text.Length - a.Length - 1, a.Length + 1);    //This line is never reached because "Cast" can only be at the beginning of a command. I'm leaving it here in case I decide to change that
                            autoCast = true;
                            break;
                        }
                    }

                //Power Cast
                if (Command == e.Result.Text)
                    foreach (string a in powerCast)
                    {
                        if (e.Result.Text.StartsWith(a + '\t'))
                        {
                            Command = e.Result.Text.Remove(0, a.Length);
                            autoCast = true;
                            break;

                        }
                        else if (e.Result.Text.EndsWith('\t' + a))
                        {
                            Command = e.Result.Text.Remove(e.Result.Text.Length - a.Length - 1, a.Length + 1);    //This line is never reached because "Cast" can only be at the beginning of a command. I'm leaving it here in case I decide to change that
                            autoCast = true;
                            break;
                        }
                    }

                Title = e.Result.Grammar.Name;

                if (Title == Command)
                    Title = GetProgression(Command, "none").Name;

                info = Title.Split('\t');

                if (info.Length >= 1)
                {
                    name = info[0];
                    if (info.Length >= 2)
                    {
                        id = info[1];
                        if (info.Length >= 3)
                        {
                            type = info[2].ToLower();
                            if (info.Length >= 4)
                                from = info[3];
                            if (info.Length >= 5)
                                autoHand = info[4];
                        }
                    }
                }

                //Assign Some Settings
                switch (name.ToLower())
                {
                    case "clear hands":
                        type = "Clear Hands";
                        from = "Skyrim.esm";
                        id = "0";
                        break;

                    case "clear shout":
                        type = "Clear Shout";
                        from = "Skyrim.esm";
                        id = "0";
                        break;
                }

                if (VocalPushToSpeak)
                {
                    if (Title == "Vocal Command Toggle - Enable")
                    {
                        if (!VocalPushToSpeak_Val)
                        {
                            VocalPushToSpeak_Val = true;
                            sound_start_listening.Play();
                            Log.Activity("Recognized: \"" + e.Result.Text + "\n    Info: \"" + Title + "\" - " + DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss.fff") + "\n");
                        }
                        return;

                    }
                    else if (Title == "Vocal Command Toggle - Disable")
                    {
                        if (VocalPushToSpeak_Val)
                        {
                            sound_stop_listening.Play();
                            VocalPushToSpeak_Val = false;
                            Log.Activity("Recognized: \"" + e.Result.Text + "\n    Info: \"" + Title + "\" - " + DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss.fff") + "\n");
                        }
                        return;
                    }

                    //Determines if commands are enabled
                    if (!VocalPushToSpeak_Val)
                        return;

                }//End Vocal Push-to-Speak

                if (Title.EndsWith("keybind"))
                {
                    name = "keybind";
                    from = "Skyrim.esm";


                    id = ToSkyrimKeyCode(id).ToString();

                    if (type.ToUpper() == "HOLD")
                    {
                        keyDuration = -1;

                    }
                    else if (type.ToUpper() == "RELEASE")
                    {
                        keyDuration = -2;
                    }
                    else
                        keyDuration = Int16.Parse(type);

                    type = "keybind";

                }
                else if (Title.EndsWith("console"))
                {
                    from = "Skyrim.esm";
                    type = "console";

                    id = "0";

                }
                else if (Title.EndsWith("location"))
                {
                    from = "Skyrim.esm";
                    type = "location";

                    id = "0";

                }
                else if (Title == "")
                { //This means it was from the full dictation grammar
                    Log.Activity("Something went wrong and there is no title for this command (\"" + e.Result.Text.ToString() + "\"). (Method: Recognizer_SpeechRecognized)", Log.LogType.Error);

                }//End if else


                //There are no special commands for shouts, so they are set to the default
                if (type.ToLower() == "shout")
                {
                    autoCast = AutoCastShouts;  //Set autocast

                    //Get Shout Level
                    hand = e.Result.Text.ToString().Split(' ').Length - 1;

                }
                else if (type.ToLower() == "power" && !autoCast)    //If autocast is true, then the term "Cast" was removed from the command by either "Hand - Cast" or "Power - Cast"
                    autoCast = AutoCastPowers;                  //Which one doesn't because because, if it was "Hand - Cast", then that option also exists in "Power - Cast"
                                                                //Otherwise it wouldn't have been in the command in the first place

                if (hand == -1)
                    switch (autoHand.ToLower())
                    {
                        case "left":
                            hand = 0;
                            break;

                        case "right":
                            hand = 1;
                            break;

                        case "both":
                            hand = 2;
                            break;

                        case "duel":
                            hand = 2;
                            break;

                        case "cast":
                            autoCast = true;
                            break;

                        case "ignore":
                            autoCast = false;
                            break;
                    }

                if (hand == -1)
                    switch (DefaultHand)
                    {
                        case "Left":
                            hand = 0;
                            break;

                        case "Right":
                            hand = 1;
                            break;

                        case "Both":
                            hand = 2;
                            break;
                    }

                //Print the information to the Command file
                commandNum++;

                //Log.Activity("ID: " + id);
                //Log.Activity("key duration: " + keybind[1]);
                //Log.Activity("Type: " + type.ToLower());
                //Log.Activity("From: " + from);
                //Log.Activity("hand: " + hand);
                //Log.Activity("autocast: " + autoCast);
                //Log.Activity("morph: " + Morph);

                if (type.ToLower() != "keybind" && type.ToLower() != "console")
                {
                    Text = name.ToLower() + '\n' +
                           Convert.ToInt64(id, 16) + '\n' +
                           "0" + '\n' +
                           type.ToLower() + '\n' +
                           from.ToLower() + '\n' +
                           hand + '\n' +
                           Convert.ToInt16(autoCast) + '\n' +
                           Morph.ToLower() + '\n' +
                           commandNum;

                }
                else if (type.ToLower() == "console")
                {

                    Text = name.Trim(' ').Replace(" +", "+").Replace("+ ", "+").Replace(" =", "=").Replace("= ", "=").Replace("* ", "*").Replace(" *", "*").ToLower() + '\n' +
                           Convert.ToInt64(id, 16) + '\n' +
                           "0" + '\n' +
                           type.ToLower() + '\n' +
                           from.ToLower() + '\n' +
                           hand + '\n' +
                           Convert.ToInt16(autoCast) + '\n' +
                           Morph.ToLower() + '\n' +
                           commandNum;
                }
                else
                {
                    Text = name.ToLower() + '\n' +
                       id + '\n' +
                       keyDuration + '\n' +
                       type.ToLower() + '\n' +
                       from.ToLower() + '\n' +
                       hand + '\n' +
                       Convert.ToInt16(autoCast) + '\n' +
                       Morph.ToLower() + '\n' +
                       commandNum;
                }

                /*//debug1//*/
                server.SendMessage(Text);
                /*//debug2//*/


                if (!VocalPushToSpeak || VocalPushToSpeak_Val)
                    Log.Activity("Recognized: \"" + e.Result.Text + "\"");

                if (type == "keybind")
                    Log.Activity("    Info: \"" + id + '\t' + type + '\t' + from + "\" - " + DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss.fff") + " (" + Math.Round((DateTime.Now - latency).TotalSeconds, 4) + "s)" + "\n");
                else
                    Log.Activity("    Info: \"" + name + '\t' + id + '\t' + type + '\t' + from + "\" - " + DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss.fff") + " (" + Math.Round((DateTime.Now - latency).TotalSeconds, 4) + "s)" + "\n");
                /*//debug1//*/
            }
            catch (Exception EX) { Log.Activity("Error in \"Recognizer_SpeechRecognized\":\n" + EX.ToString(), Log.LogType.Error); }
            /*//debug2//*/
        }//End SpeechRecognized

        #endregion

        #region Functional Methods

        /// <summary>
        /// Initializes and launches websocket server
        /// </summary>
        /// <param name="ip">Target IP address (or use localhost)</param>
        /// <param name="port">Target websocket port</param>
        private static void StartWebsocketServer(string ip, int port)
        {
            // Handle case where server is already active (terminate it)
            if (server != null)
            {
                server.OnConnected -= ClientConnected; // Unsubscribe to OnConnected event
                server.OnDisconnected -= ClientDisconnected; // Unsubscribe to OnDisconnected event
                server.OnMessageReceived -= ClientMessageReceived; // Unsubscribe to OnMessageRecieved event (from client)
                server.DisposeServer();
                server = null;
            }

            // Initialize WebSocketServer
            server = new WebSocketServer(ip, port);
            if (server.StartServer() == false) // Attempt to start WebSocketServer and react if process fails
            {
                /// *** Do stuff in response to WebSocketServer failing to start
                Log.Debug("Application failed to start websocket server", Log.LogType.Error);
                return;
            }
            else
            {
                server.OnConnected += ClientConnected; // Subscribe to OnConnected event
                server.OnDisconnected += ClientDisconnected; // Subscribe to OnDisconnected event
                server.OnMessageReceived += ClientMessageReceived; // Subscribe to OnMessageRecieved event (from client)
            }
        }

        /// <summary>
        /// Watch and apply changes in target websocket port configuration file
        /// </summary>
        private static void ConfigureWebsocketPort()
        {
            // Define directory and file to track
            string portConfigDirectory = @"Data\SKSE\Plugins\VOX\Websocket Configuration";
            string portConfigFile = "PortConfig.txt";
            Log.Debug($"Watching websocket port configuration file", Log.LogType.Info);
            string portConfigFilePath = Path.Combine(portConfigDirectory, portConfigFile);

            // Capture data from target file (if it exists) and act on it (if applicable)
            try
            {
                if (File.Exists(portConfigFilePath) == true)
                {
                    string content = File.ReadAllText(portConfigFilePath);
                    if (Int32.TryParse(content, out int port) == true && port >= 1050 && port <= 65500)
                    {
                        websocketPort = port;
                        Log.Debug($"Imported websocket port configuration from SKSE plugin: {websocketPort}", Log.LogType.Info);
                    }
                    else
                    {
                        Log.Debug($"Imported websocket port ({port}) is not a valid integer", Log.LogType.Error);
                        if (websocketPort == defaultWebsocketPort)
                            Log.Debug($"Websocket server will continue leveraging default port (no change)", Log.LogType.Info);
                        else
                        {
                            Log.Debug($"Websocket server will leverage default port: {defaultWebsocketPort}", Log.LogType.Info);
                            websocketPort = defaultWebsocketPort;
                            StartWebsocketServer(ip, websocketPort);
                        }
                    }
                }
                else
                    Log.Debug($"Websocket port configuration file does not exist", Log.LogType.Error);
            }
            catch (Exception ex)
            {
                Log.Debug($"Error retrieving starting websocket configuration from file: {ex.Message}", Log.LogType.Error);
            }

            // Watch the target file for changes
            try
            {
                // Create a new FileSystemWatcher and set its properties
                portConfigWatcher = new FileSystemWatcher();
                portConfigWatcher.Path = portConfigDirectory;
                portConfigWatcher.NotifyFilter = NotifyFilters.LastAccess | NotifyFilters.LastWrite; // Watch for changes in LastAccess and LastWrite times

                // Only watch specific text file
                portConfigWatcher.Filter = portConfigFile;

                // Add event handlers
                portConfigWatcher.Changed += new FileSystemEventHandler(OnPortConfigChanged);
                portConfigWatcher.Created += new FileSystemEventHandler(OnPortConfigChanged);

                // Begin watching
                portConfigWatcher.EnableRaisingEvents = true;
            }
            catch (Exception ex)
            {
                Log.Debug($"Error watching websocket configuration file: {ex.Message}", Log.LogType.Error);
            }
        }

        /// <summary>
        /// Gracefully clean up application
        /// </summary>
        private static void CleanUpApplication()
        {
            Log.Debug("Cleaning up before exit", Log.LogType.Info);

            try
            {

                #region Clean Up FileSystemWatcher

                if (portConfigWatcher != null)
                {
                    portConfigWatcher.EnableRaisingEvents = false;
                    portConfigWatcher.Changed -= OnPortConfigChanged;
                    portConfigWatcher.Created -= OnPortConfigChanged;
                    portConfigWatcher.Dispose();
                    portConfigWatcher = null;
                }

                #endregion

                #region Clean Up ProcessWatcher

                ProcessWatcher.ProcessClosed -= OnProcessClosed;
                ProcessWatcher.TerminateProcessWatcher();

                #endregion

                #region Clean Up WebSocket

                if (server != null)
                {
                    server.OnConnected -= ClientConnected;
                    server.OnDisconnected -= ClientDisconnected;
                    server.OnMessageReceived -= ClientMessageReceived;
                    server?.DisposeServer();
                    server = null;
                }

                #endregion

                #region Clean Up Speech Recognition Engine

                if (recognizer != null)
                {
                    recognizer.SpeechRecognized -= Recognizer_SpeechRecognized;
                    recognizer?.Dispose();
                    recognizer = null;
                }

                #endregion

                #region Remove Modification to Speech Dictionary 

                // Remove the dragon shout words from the speech dictionary
                dictionaryInterface.ModifyDictionaryDataAsync(DictionaryInterface.DictionaryAction.Remove, false, "Skyrim Dictionary Training.txt", true);

                #endregion

                #region Final Logging

                Log.Debug("Application clean up finished. Application closing now.", Log.LogType.Info);

                //Needs to be adjusted to the new logging system, but I'm not sure what this is supposed to do
                /* 
                 if (logOutput == log.LogOutput.Console)
                {
                    Console.WriteLine();
                    Log.Debug("Press any key to exit", Log.LogType.Info);
                    Console.ReadLine();
                }*/

                #endregion

            }
            catch (Exception ex)
            {
                Log.Debug($"Error during application clean up: {ex.Message}", Log.LogType.Error);
            }
        }

        /// <summary>
        /// Launch websocket server port configurator
        /// </summary>
        private static void LaunchPortConfigurator()
        {
            PortConfigurator portConfigurator; // Declare portConfigurator
            Thread t = null; // Initialize Thread t
            try // Attempt the following code...
            {
                int currentPort = websocketPort;
                int newPort = -1;
                t = new Thread(new ThreadStart(() => // Create new Thread that executes the following...
                {
                    portConfigurator = new PortConfigurator(websocketPort); // Create new PortConfigurator instance
                    portConfigurator.ShowDialog(); // Show portConfigurator window
                    if (portConfigurator.Port != -1)
                        newPort = portConfigurator.Port;
                }));
                t.SetApartmentState(ApartmentState.STA); // Set thread apartment state
                t.IsBackground = true; // Set thread as background
                t.Start(); // Start thread execution
                t.Join(); // Block main thread while thread t executes
                if (newPort == -1)
                    Log.Debug($"Websocket port configuration canceled", Log.LogType.Info);
                else if (newPort != websocketPort)
                {
                    Log.Debug($"Setting websocket server port to {newPort}", Log.LogType.Error);
                    websocketPort = newPort;
                    StartWebsocketServer(ip, websocketPort); // (Re)launch websocket server with new port configuration
                }
                else
                    Log.Debug($"Specified port is already being used by VOX websocket server", Log.LogType.Info);
            }
            catch (Exception ex) // Handle exceptions encountered in above code
            {
                Log.Debug($"Error encountered during port configurator thread processing: {ex.Message}", Log.LogType.Error); // Log error and report to event log
            }
            finally // Perform after try-catch
            {
                t?.Abort(); // Attempt to abort thread if it isn't null (this is an extra safety)
                t = null; // Set thread t as null for cleanup
                portConfigurator = null; // Set dictionaryEditor as null for cleanup
            }
        }

        /// <summary>
        /// Launch Windows Speech Recognition Menu
        /// </summary>
        private static void LaunchSpeechRecognitionMenu()
        {
            Process proc = new Process();
            proc.StartInfo.FileName = Path.Combine(Environment.SystemDirectory, "control.exe");
            proc.StartInfo.Arguments = "/name Microsoft.SpeechRecognition";
            proc.Start();

            int counter = 0;
            IntPtr handle = WinGetHandle("Speech Recognition");
            while (true)
            {
                if (handle != IntPtr.Zero)
                    break;
                else
                {
                    if (counter++ < 2)
                    {
                        Thread.Sleep(5000);
                        handle = WinGetHandle("Speech Recognition");
                    }
                    else
                    {
                        MessageBoxButtons buttons = MessageBoxButtons.OK;
                        string title = "VOX - Speech Recognition Menu Issue";
                        string message = "An issue was encountered while launching the Speech Recognition Menu";
                        MessageBox.Show(message, title, buttons, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button1, (MessageBoxOptions)0x40000);
                        return;
                    }
                }
            }
            SetWindowPos(handle, new IntPtr(-1), 0, 0, 0, 0, 0x1 | 0x2); // Set Windows Speech Recognition menu window as TopMost
        }

        /// <summary>
        /// Retrieves handle of window based on window title
        /// </summary>
        /// <param name="windowTitleName">Name of window title of interest</param>
        /// <returns>IntPtr representing target window handle</returns>
        private static IntPtr WinGetHandle(string wName)
        {
            foreach (Process pList in Process.GetProcesses())
            {
                if (pList.MainWindowTitle.Contains(wName))
                    return pList.MainWindowHandle;
            }
            return IntPtr.Zero;
        }

        /* /// <summary>
        /// Directly launch Windows speech recognition training dialog
        /// </summary>
        private static void LaunchSpeechRecognitionTraining()
        {
            IntPtr ptr = new IntPtr();
            ///Wow64DisableWow64FsRedirection(ref ptr); //attempt to disable redirection
            if (System.IO.File.Exists(System.IO.Path.Combine(Environment.SystemDirectory, "Speech", "SpeechUX", "SpeechUXWiz.exe")))
            {
                String strDefaultTokenId = (String)Microsoft.Win32.Registry.CurrentUser.OpenSubKey("Software").OpenSubKey("Microsoft").OpenSubKey("Speech").OpenSubKey("RecoProfiles").GetValue("DefaultTokenId", null);
                System.Diagnostics.ProcessStartInfo psi = new System.Diagnostics.ProcessStartInfo();
                psi.FileName = System.IO.Path.Combine(Environment.SystemDirectory, "Speech", "SpeechUX", "SpeechUXWiz.exe");
                psi.Arguments = String.Format("UserTraining,{0},{1},0,0,\"\"", recognizer.RecognizerInfo.Culture.IetfLanguageTag, strDefaultTokenId);
                System.Diagnostics.Process p = new System.Diagnostics.Process();
                p.StartInfo = psi;
                p.Start();
            }
            ///Wow64RevertWow64FsRedirection(ptr); //re-enable redirection            
        } */

        #endregion

        #region External Methods

        /*[DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool Wow64RevertWow64FsRedirection(IntPtr ptr);

        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool Wow64DisableWow64FsRedirection(ref IntPtr ptr); */

        [DllImport("user32")]
        private static extern bool SetWindowPos(IntPtr hwnd, IntPtr hwnd2, int x, int y, int cx, int cy, int flags);

        #endregion

    }
}
