// Class for manipulating the speech dictionary

using SpeechLib;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using Utilities;

namespace SpeechDictionaryTools
{
    public class DictionaryInterface
    {

        #region Fields

        ///private bool stopProcessing = false; // Boolean flag for terminating DictionaryInterface processing
        private readonly CancellationTokenSource cts = new CancellationTokenSource();
        private int langID = new System.Globalization.CultureInfo("en-US").LCID; // Obtain the language ID
        private SpLexicon lex; // Declare SpLexicon for accessing SAPI (speech) dictionary functionality

        #endregion

        #region Enumeration

        // Enumeration for dictionary actions
        public enum DictionaryAction
        {
            Empty,
            Get,
            Add,
            Remove,
        }

        #endregion

        #region Properties

        // Storage of all dictionary words
        private SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)> _dictionaryWords = new SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)>();
        public SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)> DictionaryWords
        {
            get { return _dictionaryWords; } // Return _dictionaryWords
            private set { _dictionaryWords = value; } // Set _dictionaryWords as passed-in value
        }

        #endregion

        #region Constructor

        /// <summary>
        /// Constructur for DictionaryInterface
        /// </summary>
        /// <param name="action">Requested DictionaryAction</param>
        public DictionaryInterface()
        {
            GetDictionaryDataAsync(); // Asynchronously obtain speech dictionary data for internal processing
        }

        #endregion

        #region Speech Dictionary Data Retrieval and Manipulation

        /// <summary>
        /// Retrieve all speech dictionary data with asynchronous query
        /// </summary>
        /// <returns>Boolean indicating processing success state</returns>
        public bool? GetDictionaryDataAsync()
        {
            ///TrackingStopwatch w = new TrackingStopwatch(VA, "Get Dictionary Words Async"); // Create new TrackingStopwatch instance for speech dictionary retrieval
            bool? result = false; // Initialize boolean for storing processing result
            try // Attempt the following coded
            {
                lex = new SpLexicon(); // Create new SpLexicon instance
                int myGenerationID = lex.GenerationId; // Get the GenerationId (kind of like a version) of the current user dictionary
                ISpeechLexiconWords lexWords = lex.GetWords(SpeechLexiconType.SLTUser, out myGenerationID); // Construct a list of all words in the current user lexicon
                if (_dictionaryWords != null) // Check if current index of dictionary words is NOT empty
                    this._dictionaryWords.Clear(); // Clear all data from dictionaryWords
                if (lexWords.Count > 0) // Check if there are word entries in the speech dictionary
                {
                    var getDictionaryTaskList = new List<Task<(string word, string pronunciation, SpeechPartOfSpeech speechPart)>>(); // Create new List of tasks (with tupple for output)
                    foreach (ISpeechLexiconWord lexWord in lexWords) // Loop through all items in lexWords
                    {
                        var task = GetDictionaryWordData(lexWord); // Create a new task to obtain dictionary data for lexWord
                        getDictionaryTaskList.Add(task); // Store task in list of tasks
                    }
                    Task.WhenAll(getDictionaryTaskList).Wait(cancellationToken: cts.Token); // Execute all the dictionary data retrieval tasks concurrently
                    //if (stopProcessing == true) // Check if processing termination request was triggered
                    //    return null; // Return from this method
                    foreach (var task in getDictionaryTaskList) // Loop through all tasks in getDictionaryTaskList
                    {
                        if (string.IsNullOrEmpty(task.Result.word) == false) // Check if resulting word is valid (not null or empty)
                            DictionaryWords[task.Result.word] = (task.Result.pronunciation, task.Result.speechPart); // Add word and associated data to DictionaryWords
                    }
                }
                if (this.DictionaryWords.Count > 0) result = true;
            }
            catch (Exception ex) // Handle errors encountered in above code
            {
                result = null; // Set boolean flag indicating unsuccessful processing of speech dictionary retrieval (error encountered)
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///    Logging.ReportAndLogError(ex, $"Error retrieving speech dictionary data", VA); // Log error and report to event log
            }
            finally
            {
                ///w.Watch("Get Dictionary Words Async"); // Report TrackingStopwatch
            }
            return result; // Return result
        }

        /// <summary>
        /// Async task for retrieving speech dictionary data for a single word
        /// </summary>
        /// <param name="lexWord">ISpeechLexiconWord representing target word</param>
        /// <param name="action">Requested DictionaryAction</param>
        /// <returns>Tuple containing target word data from speech dictionary</returns>
        private async Task<(string word, string pronunciation, SpeechPartOfSpeech speechPart)> GetDictionaryWordData(ISpeechLexiconWord lexWord)
        {
            await Task.Delay(1); // Await brief task to trigger parallel processing (if multiple words are being queried in calling method)
            string word = null; // Initialize string for storing current speech dictionary word
            string pronunciation; // Declare string for storing word pronunciation
            SpeechPartOfSpeech speechPart; // Declare variable for storing word part of speech
            try // Attempt the following code
            {
                word = lexWord.Word; // Transfer word data
                try { pronunciation = lexWord.Pronunciations.Item(0).Symbolic ?? ""; } // Try to extract phonetic pronunciation data for word (store as blank if null)
                catch { pronunciation = ""; } // If "try" encounters error set pronunciation to a blank character
                try { speechPart = lexWord.Pronunciations.Item(0).PartOfSpeech; } // Try to extract part of speech data for word
                catch
                {
                    /// if (string.IsNullOrEmpty(pronunciation) && (action == DictionaryAction.Add || action == DictionaryAction.Get)) // Check if pronunciation data is empty and details should be shown
                    /// Logging.OutputToLog($"\"{word}\" is missing pronunciation data. This may yield unexpected results.", VA, "yellow"); // Output info to event log
                    speechPart = SpeechPartOfSpeech.SPSUnknown; // Set speechPart as "unknown" (not sure if execution will ever make it to this line)
                }
            }
            catch (Exception ex) // Handle errors encountered in above code
            {
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///Logging.ReportAndLogError(ex, $"Error retrieving speech dictionary data{(word != null ? $" for {word}" : "")}", VA); // Log error and report to event log
                return (null, null, SpeechPartOfSpeech.SPSFunction);
            }
            return (word, pronunciation, speechPart); // Return dictionary word data
        }

        /// <summary>
        /// Modify speech dictionary based on inputted data dictionary asynchronously
        /// </summary>
        /// <param name="action">Requested DictionaryAction</param>
        /// <param name="showDetail">Indicates if processing details should be outputted to log</param>
        /// <param name="trainingFileName">Name of embedded speech dictionary training text file</param>
        /// <param name="caseInsensitiveWords">Indicates if processing of word data should be case-insensitve</param>
        /// <returns>Boolean indicating processing success state</returns>
        public bool? ModifyDictionaryDataAsync(DictionaryAction action, bool showDetail, string trainingFileName, bool caseInsensitiveWords)
        {
            ///TrackingStopwatch w = new TrackingStopwatch(VA); // Create new (empty) TrackingStopwatch instance
            ///w.Watch("Speech Dictionary Modification Total"); // Track total speech dictionary modification (debug)
            bool? result = false; // Initialize boolean for storing processing result
            try
            {
                if (LoadDictionaryModificationData(action, showDetail, out SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)> modDictionary, trainingFileName, caseInsensitiveWords) == true) // Load text file and check if process was successful
                {
                    var modifyDictionaryTaskList = new List<Task<(string message, string color)>>(); // Create new List of tasks
                    ///w.Watch("Speech Dictionary Modification"); // Create new TrackingStopwatch instance for speech dictionary modification
                    //w.Watch("Modification Task Setup");
                    if (action == DictionaryAction.Add) // Check if dictionary modification should involve adding word data
                    {
                        try // Attempt the following code...
                        {
                            foreach (var dictionaryItem in modDictionary) // Loop through each dictionary item in modDictionary
                            {
                                //if (stopProcessing == true) // Check if processing termination request was triggered
                                //    return null; // Return from this method
                                string[] modWord; // Declare string array for storing modification words
                                if (caseInsensitiveWords == true) // Check if lower AND uppercase modifications should be made
                                {
                                    modWord = new string[2]; // Define size of modWord
                                    modWord[0] = dictionaryItem.Key.ToLower(); // Set modWord's first index as input word converted to all lowercase
                                    modWord[1] = FirstLetterToUppercase(modWord[0]); // Set modWord's second index as input word converted to FirstLetterUppercase
                                }
                                else // Process the word of interest "as-is"
                                {
                                    modWord = new string[] { dictionaryItem.Key }; // Initialize modWord
                                    if (HasUpperCase(modWord[0], 0) == true) // Check if modWord content has an uppercase first letter
                                        modWord[0] = FirstLetterToUppercase(modWord[0]); // Convert modWord to FirstLetterUppercase
                                    else
                                        modWord[0] = modWord[0].ToLower(); // Convert modWord to lowercase
                                }
                                foreach (string word in modWord) // Loop through modWord content
                                {
                                    var task = AddDictionaryDataAsync(word, dictionaryItem.Value.Pronunciation, dictionaryItem.Value.SpeechPart.ToString(), showDetail); // Create a new task to add dictionary data for dictionaryItem
                                    modifyDictionaryTaskList.Add(task); // Store task in list of tasks
                                }
                            }
                            ///w.Watch("Modification Task Setup"); // Report TrackingStopwatch
                            Task.WhenAll(modifyDictionaryTaskList).Wait(cancellationToken: cts.Token); // Execute all the dictionary data modification tasks concurrently
                            foreach (var task in modifyDictionaryTaskList) // Loop through each task in modifyDictionaryTaskList
                            {
                                if (task.Result.message != null) // Check if result key is NOT null
                                    Log.Debug(task.Result.message, Log.LogType.Error);
                                    ///Logging.WriteToLog(task.Result.message, Logging.LogType.Error); // Output info to event log
                            }
                            ///w.Watch("Speech Dictionary Modification"); // Report TrackingStopwatch (debug)
                            result = true; // Set boolean flag indicating successful processing of speech dictionary modification

                        }
                        catch (Exception ex) // Handle errors encountered in above code
                        {
                            result = null; // Set boolean flag indicating unsuccessful processing of dictionary modification request (error encountered)
                            ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                            ///Logging.ReportAndLogError(ex, $"Error during dictionary modification (word removal)", VA); // Log error and report to event log
                        }
                    }
                    else if (action == DictionaryAction.Remove) // Check if dictionary modification should involve removing word data
                    {
                        try // Attempt the following code...
                        {
                            foreach (var dictionaryItem in modDictionary) // Loop through each dictionary item in modDictionary
                            {
                                //if (stopProcessing == true) // Check if processing termination request was triggered
                                //    return null; // Return from this method
                                string[] modWord; // Declare string array for storing modification words
                                if (caseInsensitiveWords == true) // Check if lower AND uppercase modifications should be made
                                {
                                    modWord = new string[2]; // Define size of modWord
                                    modWord[0] = dictionaryItem.Key.ToLower(); // Set modWord's first index as input word converted to all lowercase
                                    modWord[1] = FirstLetterToUppercase(modWord[0]); // Set modWord's second index as input word converted to FirstLetterUppercase
                                }
                                else // Process the word of interest "as-is"
                                {
                                    modWord = new string[] { dictionaryItem.Key }; // Initialize modWord
                                    if (HasUpperCase(modWord[0], 0) == true) // Check if modWord content has an uppercase first letter
                                        modWord[0] = FirstLetterToUppercase(modWord[0]); // Convert modWord to FirstLetterUppercase
                                    else
                                        modWord[0] = modWord[0].ToLower(); // Convert modWord to lowercase
                                }
                                foreach (string word in modWord) // Loop through modWord content
                                {
                                    var task = RemoveDictionaryDataAsync(word, showDetail); // Create a new task to remove dictionary data for dictionaryItem
                                    modifyDictionaryTaskList.Add(task); // Store task in list of tasks
                                }
                            }
                            ///w.Watch("Modification Task Setup"); // Report TrackingStopwatch
                            Task.WhenAll(modifyDictionaryTaskList).Wait(cancellationToken: cts.Token); // Execute all the dictionary data modification tasks concurrently
                            foreach (var task in modifyDictionaryTaskList) // Loop through each task in modifyDictionaryTaskList
                            {
                                if (task.Result.message != null) // Check if result key is NOT null
                                    Log.Debug(task.Result.message, Log.LogType.Error);
                                    ///Logging.WriteToLog(task.Result.message, Logging.LogType.Error); // Output info to event log
                            }
                            ///w.Watch("Speech Dictionary Modification"); // Create new TrackingStopwatch instance for speech dictionary modification
                            result = true; // Set boolean flag indicating successful processing of speech dictionary modification
                        }
                        catch (Exception ex) // Handle errors encountered in above code
                        {
                            result = null; // Set boolean flag indicating unsuccessful processing of dictionary modification request (error encountered)
                            ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                            ///Logging.ReportAndLogError(ex, $"Error during dictionary modification (word removal)", VA); // Log error and report to event log
                        }
                    }
                    //else
                    //    Logging.OutputToLog($"Dictionary action {action} not valid when modifying speech dictionary. Dictionary modification canceled.", VA, "red"); // Output info to event log
                }
            }
            catch (Exception ex)
            {
                result = null; // Set boolean flag indicating unsuccessful processing of speech dictionary modification (error encountered)
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///Logging.ReportAndLogError(ex, $"Error modifying speech dictionary", VA); // Log error and report to event log
            }
            finally
            {
                /// w.Watch("Speech Dictionary Modification Total"); // Report TrackingStopwatch (debug)
            }
            GetDictionaryDataAsync(); // Refresh the DictionaryWords
            ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
            ///Logging.OutputToLog($"Speech dictionary data {(action == DictionaryAction.Add ? "addition" : "removal")} complete", VA, "blue"); // Output info to event log
            return result; // Return result
        }

        /// <summary>
        /// Import speech dictionary data from text file
        /// </summary>
        /// <param name="action">Requested DictionaryAction</param>
        /// <param name="showDetail">Indicates if processing details should be outputted to log</param>
        /// <param name="modDictionary">Outputted SortedDictionary containing modification words and associated data</param>
        /// <param name="trainingFileName">Name of embedded speech dictionary training text file</param> 
        /// <param name="caseInsensitiveWords">Indicates if processing of word data should be case-insensitve </param>
        /// <returns>Boolean indicating processing success state</returns>
        private bool? LoadDictionaryModificationData(DictionaryAction action, bool showDetail, out SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)> modDictionary, string trainingFileName, bool caseInsensitiveWords)
        {
            ///TrackingStopwatch w = new TrackingStopwatch(VA); // Create new (empty) TrackingStopwatch instance
            /// w.Watch("Import Dictionary Modification File"); // Track importing dictionary modification file (debug)
            bool? result = false; // Initialize boolean for storing processing result
            modDictionary = new SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)>(StringComparer.InvariantCultureIgnoreCase); // Initialize modDictionary for storing loaded entries from import file
            ///w.Watch("Load Modification File"); // Track loading dictionary modification file (debug)
            try // Attempt the following code...
            {
                var assembly = Assembly.GetExecutingAssembly(); // Retrieve assembly information
                string resourcePath = assembly.GetManifestResourceNames().Single(str => str.EndsWith(trainingFileName)); // Obtain path to embedded resource (text file)
                using (Stream stream = assembly.GetManifestResourceStream(resourcePath)) // Load embedded resource as new Stream instance
                using (StreamReader sr = new StreamReader(stream)) // Create StreamReader instance for reading the embedded resource
                {
                    string importLine = String.Empty; // Initialize string for storing text lines from the dictionary import file
                    int importLineCounter = 1; // Initialize import file line item counter
                    int typeCount = 3; // Initialize integer for maximum number of delimited sub-entries (data columns) for all line items in the dictionary import file
                    string[] preData = new string[typeCount]; // Initialize string array for holding dictionary import file data before final processing
                    try // Attempt the following code...
                    {
                        while ((importLine = sr.ReadLine()) != null) // Loop while there are lines to read in the dictionary import file and store each line's content in importLine
                        {
                            ///if (stopProcessing == true) return null; // Check if processing should stop and react accordingly
                            if (!string.IsNullOrWhiteSpace(importLine)) // Check if the read line (stored in importLine) is NOT blank (i.e., it contains characters of interest)
                            {
                                string[] items = Array.ConvertAll(importLine.Split(';'), p => p.Trim()); // Separate the text delimited by the ";" and apply trimming and store the separated data in a string array
                                if (items.Count() > 3)
                                {
                                    ///Logging.OutputToLog($"Cannot import data with more then 3 entries within a single text line. Skipping import of text file line {importLineCounter++}.", VA, "red"); // Output info to event log
                                    continue; // Continue back to top of parent "while" loop
                                }
                                int itemCount = Math.Min(items.Length, typeCount); // Get the number of delimited entries for the current line item
                                string processItem = null; // Initialize string variable for storing (column) entry for current line item
                                bool itemOverwrite = false; // Initialize boolean flag for indicating if current item needs to overwrite a previously loaded entry
                                Array.Clear(preData, 0, preData.Length); // Empty the preData array
                                if (itemCount == 2) preData[2] = "SPSUnknown"; // Set PartOfSpeech as unknown if only 2 items are inputted
                                for (int i = 0; i < itemCount; i++) // Loop through line item's content based on itemCount
                                {
                                    try // Attempt the following code...
                                    {
                                        processItem = items[i]; // Transfer item of interest
                                        if (i == 0) // Check if item index is 0 (first item in the importLine content, which should be a WORD)
                                        {
                                            if (!string.IsNullOrWhiteSpace(processItem)) // Check if current item is NOT blank
                                            {
                                                if (processItem.Contains("//") == true) // Check if commenting "//" are present in processItem
                                                {
                                                    if (processItem.Substring(0, 2) == "//") // Check if current word (has a leading "//" which denotes it to be "commented out" in the modification file and should be skipped)
                                                    {
                                                        //if (showDetail == true) // Check if user wants to show processing detail in the VoiceAttack event log
                                                        //{
                                                        //    Thread.Sleep(visualPause); // Brief pause to ensure VoiceAttack log output sequentially updates (for visual purposes only)
                                                        //    Logging.OutputToLog($"Skipping import of commented text file line {importLineCounter}", VA, "pink"); // Output info to event log
                                                        //}
                                                        break; // Break out of parent "for" loop
                                                    }
                                                    else // "//" characters are (hopefully) after the text of interest (traditional comment position)
                                                        processItem = processItem.Remove(processItem.IndexOf("//")).Trim(); // Remove commenting "//" characters and subsequent comment text
                                                }
                                                //if (items.Length > typeCount) // Check if the number of data entries in the current line item is more than expected
                                                //    Logging.OutputToLog($"Import text file line {importLineCounter} (\"{processItem}\") has more than three word data types", VA, "yellow"); // Output info to event log
                                                if (itemCount == 1 && action == DictionaryAction.Add) // Check if there is only one data entry in the current line item AND requested speech dictionary action is to "add data"
                                                {
                                                    ///Logging.OutputToLog($"Missing required word data for \"{processItem}.\" Skipping import of text file line {importLineCounter}.", VA, "red"); // Output info to event log
                                                    break; // Break out of parent "for" loop
                                                }
                                                if (modDictionary.ContainsKey(processItem) == true) // Check if modDictionary already contains the word stored in processItem
                                                {
                                                    itemOverwrite = true; // Set flag indicating current item needs to overwrite a previously loaded entry
                                                    if (modDictionary[processItem].Pronunciation == items[1] && caseInsensitiveWords == true) // Check if repeat word has same pronunciation as existing entry and all word casings are being considered
                                                    {
                                                        ///Logging.OutputToLog($"{processItem} is already loaded. Skipping import of text file line {importLineCounter}.", VA, "yellow"); // Output info to event log
                                                        preData[0] = null; // Set preData[0] as null to flag for processing skip
                                                        break; // Break out of parent "for" loop
                                                    }
                                                }
                                                //if (processItem.Contains(" ") == true) // Check if there is a space in the word/phrase of interest
                                                //    Logging.OutputToLog($"\"{processItem}\" contains more than one word and may cause unexpected results", VA, "yellow"); // Output info to event log
                                            }
                                            else
                                            {
                                                ///Logging.OutputToLog($"Cannot import data that does not include a word. Skipping import of text file line {importLineCounter}.", VA, "red"); // Output info to event log
                                                break; // Break out of "for" loop
                                            }
                                        }
                                        else if (string.IsNullOrWhiteSpace(processItem) == true) // Check if current item is blank
                                        {
                                            if (i == 1) // Check if item index is 1 (second item in the importLine content, which should be pronunciation phonemes). If true then line item is missing pronunciation data.
                                            {
                                                if (itemCount == 3) // Check if itemCount is 3
                                                {
                                                    string importSpeechPart = items[2].Contains("//") ? items[2].Remove(items[2].IndexOf("//")).Trim() : items[2].Trim(); // Clean up PartOfSpeech data from line item (if needed) and store result

                                                    if (importSpeechPart == "SPSUnknown") // Check if importSpeechPart is "SPSUnknown"
                                                    {
                                                        ///Logging.OutputToLog($"\"{preData[0]}\" is missing pronunciation data in import text line {importLineCounter}. This may yield unexpected results.", VA, "yellow"); // Output info to event log (debug)
                                                        preData[i] = ""; // Set pronunciation as empty
                                                        preData[i + 1] = "SPSUnknown"; // Store "SPSUnknown" for PartOfSpeech (index 2)
                                                        break; // Break out of parent "for" loop
                                                    }
                                                    else if (importSpeechPart != "SPSSuppressWord") // Check if importSpeechPart is NOT "SPSSuppressWord"
                                                    {
                                                        // Pronunciation can only be blank if PartOfSpeech is "SPSSuppressWord" (or also "SPSUnknown")
                                                        ///Logging.OutputToLog($"No pronunciation provided for \"{preData[0]}.\" Skipping import of text file line {importLineCounter}.", VA, "red"); // Output info to event log
                                                        preData[0] = null; // Set first item of preData to null to signal that line item should not be loaded
                                                        break; // Break out of parent "for" loop
                                                    }
                                                }
                                                else // ItemCount is 2
                                                {
                                                    ///Logging.OutputToLog($"No pronunciation provided for \"{preData[0]}.\" Skipping import of text file line {importLineCounter}.", VA, "red"); // Output info to event log
                                                    preData[0] = null; // Set first item of PreData to null to signal that line item should not be loaded
                                                    break; // Break out of parent "for" loop
                                                }
                                            }
                                            else // item index is 2 (third item in the import line content, which should be PartOfSpeech). If code reaches here then this third element is missing.
                                            {
                                                ///Logging.OutputToLog($"{preData[0]} is missing PartOfSpeech data. Default of \"SPSUnknown\" will be used.", VA, "yellow"); // Output info to event log
                                                processItem = "SPSUnknown"; // Store "SPSUnknown" for PartOfSpeech (index 2)
                                            }
                                        }
                                        else
                                        {
                                            if (processItem.Contains("//") == true) // Check if commenting "//" are present in processItem
                                                processItem = processItem.Remove(processItem.IndexOf("//")).Trim(); // Remove commenting "//" characters and subsequent comment text
                                            if (i == 1) // Check if item index is 1 (second item in the importLine content, which should be pronunciation phones) 
                                            {
                                                if (items.Length == 3 && (items[2].Contains("//") ? items[2].Remove(items[2].IndexOf("//")).Trim() : items[2].Trim()) == "SPSSuppressWord") // Clean up PartOfSpeech data from line item (if needed) and check if result is "SPSSuppressWord"
                                                {
                                                    //if (string.IsNullOrEmpty(items[1]) == false) // Check if items[1] is NOT empty (pronunciation was provided)
                                                    //    Logging.OutputToLog($"\"{items[0]}\" imported as suppressed (SPSSuppressWord), and its pronunciation ({items[1]}) will be discarded", VA, "yellow"); // Output info to event log
                                                    processItem = ""; // Set processItem to (effectively) blank since suppressed words should not pass on a pronunciation
                                                }
                                            }
                                        }
                                        preData[i] = processItem; // Store processItem in preprocessing string array
                                    }
                                    catch (Exception ex) // Handle errors encountered in above code
                                    {
                                        preData[0] = null; // Set first item of PreData to null to signal that line item should not be loaded
                                        string issueSection = $"\"{processItem}\" {(i == 0 ? "word" : (i == 1 ? "pronunciation" : "PartOfSpeech"))} data"; // Capture context of encountered issue
                                        ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                                        ///Logging.ReportAndLogError(ex, $"Error importing {issueSection} at text file line {importLineCounter}", VA); // Log error and report to event log
                                        break; // Break out of parent "for" loop
                                    }
                                }
                                if (preData[0] != null) // Check if preData has valid word data that must be loaded
                                {
                                    if (Enum.TryParse(preData[2], true, out SpeechPartOfSpeech speechPart) == true) // Attempt to extract speechPart from preData
                                    {
                                        if (itemOverwrite == true) // Check if current item must overwrite a previous loaded entry of the word
                                        {
                                            ///Logging.OutputToLog($"\"{preData[0].ToLower()}\" found multiple times within import text file. Most recent data will be imported (line {importLineCounter}).", VA, "yellow"); // Output info to event log
                                            modDictionary = ChangeDictionaryKey(modDictionary, preData[0], preData[0]); // Overwrite existing modDictionary key
                                        }
                                        modDictionary[preData[0]] = (preData[1], speechPart); // Capture all preData in modDictionary
                                        //if (showDetail == true) // Check if user wants to show processing detail in the VoiceAttack event log
                                        //{
                                        //    Thread.Sleep(visualPause); // Brief pause to ensure VoiceAttack log output sequentially updates (for visual purposes only)
                                        //    Logging.OutputToLog((itemOverwrite == true ? "Overwrote " : "Loaded ") + $"\"{preData[0]}\"", VA, "pink"); // Output information to VoiceAttack event log
                                        //}
                                    }
                                    //else
                                    //    Logging.OutputToLog($"\"{preData[0]}\" part of speech data is invalid. Cannot import text file line {importLineCounter}.", VA, "red"); // Output info to event log
                                }
                            }
                            //else // importLine is blank
                            //{
                            //    if (showDetail == true) // Check if user wants to show processing detail in the VoiceAttack event log
                            //    {
                            //        Thread.Sleep(visualPause); // Brief pause to ensure VoiceAttack log output sequentially updates (for visual purposes only)
                            //        Logging.OutputToLog($"Skipping import of blank text file line {importLineCounter}", VA, "pink"); // Output info to event log
                            //    }
                            //}
                            importLineCounter++; // Increment the import file line item counter
                        }
                        result = true; // Set boolean flag indicating successful processing of import file loading
                    }
                    catch (Exception ex) // Handle errors encountered in above code
                    {
                        result = null; // Set boolean flag indicating unsuccessful processing of import file loading (error encountered)
                        ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                        ///Logging.ReportAndLogError(ex, $"Error importing speech dictionary data at text file line {importLineCounter}", VA); // Log error and report to event log
                    }
                }
            }
            catch (Exception ex) // Handle errors encountered in above code
            {
                result = null; // Set boolean flag indicating unsuccessful processing of import file loading (error encountered)
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///Logging.ReportAndLogError(ex, $"Error importing speech dictionary data from text file", VA); // Log error and report to event log
            }
            finally
            {
                ///w.Watch("Load Modification File"); // Report TrackingStopwatch (debug) 
                /// w.Watch("Import Dictionary Modification File"); // Report TrackingStopwatch (debug)
            }
            return result; // Return result
        }

        /// <summary>
        /// Add word and associated properties to speech dictionary asynchronously
        /// </summary>
        /// <param name="word">Word to add to speech dictionary</param>
        /// <param name="pronunciation">Word pronunciation</param>
        /// <param name="partOfSpeech">Word part of speech</param>
        /// <param name="showDetail">Indicates if processing details should be outputted to log</param>
        /// <returns></returns>
        public async Task<(string message, string color)> AddDictionaryDataAsync(string word, string pronunciation, string partOfSpeech, bool showDetail = false)
        {
            await Task.Delay(1); // Await brief task to trigger parallel processing (if multiple words are being queried in calling method)
            bool result = false; // Initialize boolean flag
            try // Attempt the following code...
            {
                if (Enum.TryParse(partOfSpeech, true, out SpeechPartOfSpeech speechPart) == false) // Attempt to parse partOfSpeech and check if attempt is unsuccessful
                    return ($"\"{speechPart}\" is not a valid PartOfSpeech. Cannot add \"{word}\" to speech dictionary.", "yellow)"); // Return from this method
                if (string.IsNullOrEmpty(word) == true) // Check if word is empty
                    return ($"\"{word}\" is not a valid word. Cannot add \"{word}\" to speech dictionary.", "yellow"); // Return from this method
                if (this.DictionaryWords?.ContainsKey(word) == true) // Check if requested word for addition is already in the speech dictionary
                {
                    if (this.DictionaryWords[word].Pronunciation == pronunciation && this.DictionaryWords[word].SpeechPart == speechPart) // Check if requested word for addition has same pronunciation and part of speech as what is already in the speech dictionary
                    {
                        if (string.IsNullOrEmpty(pronunciation)) // Check if pronunciation is blank
                            pronunciation = "[no pronunciation]"; // Update pronunciation
                        else
                            pronunciation = $"({pronunciation})"; // Update pronunciation
                        return ($"\"{word}\" {pronunciation} is already {(speechPart == SpeechPartOfSpeech.SPSSuppressWord ? "excluded from" : "in")} speech dictionary", "yellow"); // Return from this method (word's data and speech dictionary entry are identical, so no need for further action)
                    }
                    else
                        lex.RemovePronunciation(word, langID); // Call SAPI method for removing word data from the speech dictionary
                }
                try // Attempt the following code...
                {
                    lex.AddPronunciation(word, langID, speechPart, pronunciation); // Call SAPI method for adding word data to the speech dictionary
                    result = true; // Set boolean flag
                }
                catch (Exception ex) // Handle errors encountered in above code
                {
                    if (ex.Message == "Value does not fall within the expected range.") // Check if error message has specific content
                        return ($"\"{word}\" cannot be added to speech dictionary. Check inputted word data (most likely pronunciation issue).", "red"); // Return from this method
                    //if (ex.Message == "Value does not fall within the expected range." && stopProcessing == false) // Check if error message has specific content and processing termination request was NOT triggered
                    //    return ($"\"{word}\" cannot be added to speech dictionary. Check inputted word data (most likely pronunciation issue).", "red"); // Return from this method
                }
                if (string.IsNullOrEmpty(pronunciation) == true) pronunciation = "[no pronunciation]"; // Update pronunciation content if it is actually empty
                if (showDetail == true && result == true) return ($"\"{word}\" {(speechPart == SpeechPartOfSpeech.SPSSuppressWord ? "suppressed in" : $"({pronunciation}) added to")} speech dictionary", "gray"); // Return from this method
            }
            catch (Exception ex) // Handle errors encountered in above code
            {
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///Logging.ReportAndLogError(ex, $"Error adding \"{word}\" to speech dictionary", VA); // Log error and report to event log
            }
            return (null, null); // Return from this method
        }

        /// <summary>
        /// Remove word and associated properties from speech dictionary asynchronously
        /// </summary>
        /// <param name="word">Word to remove from speech dictionary</param>
        /// <param name="showDetail">Indicates if processing details should be outputted to log</param>
        /// <returns></returns>
        public async Task<(string message, string color)> RemoveDictionaryDataAsync(string word, bool showDetail = false)
        {
            await Task.Delay(1); // Await brief task to trigger parallel processing (if multiple words are being queried in calling method)
            try // Attempt the following code...
            {
                if (this.DictionaryWords.ContainsKey(word) == true) // Check if requested word for removal is in the speech dictionary
                {
                    lex.RemovePronunciation(word, langID); // Call SAPI method for removing word data from the speech dictionary
                    if (showDetail == true) return ($"\"{word}\" removed from speech dictionary", "gray"); // Return from this method
                }
                else
                    return ($"\"{word}\" not found in speech dictionary", "yellow"); // Return from this method
            }
            catch (Exception ex) // Handle errors encountered in above code
            {
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///Logging.ReportAndLogError(ex, $"Error removing \"{word}\" from speech dictionary", VA); // Log error and report to event log
            }
            return (null, null); // Return from this method
        }

        #endregion

        #region Other Functional Methods

        /// <summary>
        /// Voice text of interest synchronously via text-to-speech
        /// </summary>
        /// <param name="text">Text string to voice</param>
        public void VoiceText(string text)
        {
            try // Attempt the following code...
            {
                string defaultVoice; // Initialize string for storing the default text-to-speech voice
                using (System.Speech.Synthesis.SpeechSynthesizer synth = new System.Speech.Synthesis.SpeechSynthesizer()) // Create new SpeechSynthesizer instance
                {
                    synth.Volume = 100; // Set the volume level of the text-to-speech voice
                    synth.Rate = -2; // Set the rate at which text is spoken by the text-to-speech engine
                    defaultVoice = synth.Voice.Name; // Store the current default Windows text-to-speech voice
                    ///synth.SpeakAsyncCancelAll(); // Cancels all queued, asynchronous, speech synthesis operations
                    synth.SelectVoice(defaultVoice); // Set the voice for this instance of text-to-speech output to the Windows default voice
                    ///synth.SpeakAsync(text); // Generate text-to-speech output asynchronously
                    synth.Speak(text); // Generate text-to-speech output synchronously
                }
            }
            catch (Exception ex) // Handle errors encountered in above code
            {
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///Logging.ReportAndLogError(ex, $"Error during speech synthesis of {text}", VA); // Log error and report to event log
            }
        }

        /// <summary>
        /// Convert the first letter in a string to uppercase
        /// </summary>
        /// <param name="input">Input text string</param>
        /// <returns>String with first character as uppercase and everything else as lowercase</returns>
        public string FirstLetterToUppercase(string input)
        {
            try // Attempt the following code...
            {
                if (string.IsNullOrEmpty(input)) // Check if string input is null or empty
                    throw new ArgumentException("There is no first letter"); // Throw exception
                char[] a = input.ToLower().ToCharArray(); // Store the characters in input in a char array as all lowercase
                a[0] = char.ToUpper(a[0]); // Convert the first character entry in char array to uppercase
                return new string(a); // Return newly created string from char array
            }
            catch (Exception ex) // Handle errors encountered in above code
            {
                ///if (stopProcessing == false) // Check if processing termination request was NOT triggered
                ///Logging.ReportAndLogError(ex, $"Error during conversion of \"{input}\" string to uppercase format", VA); // Log error and report to event log
                return null; // Return null
            }
        }

        /// <summary>
        /// Check if any (or an index-based) character in a string is uppercase
        /// </summary>
        /// <param name="input">Text string to check</param>
        /// <param name="index">String character index to target</param>
        /// <returns>Boolean indicating if string has uppercase character</returns>
        private bool? HasUpperCase(string input, int index = -1)
        {
            if (index >= 0) // Check if an index was provided
            {
                if (index <= input.Length) // Check if provided index is within bounds of input
                    return char.IsUpper(input[index]); // Check if character at provided index is uppercase
                else
                    return null; // Return null (indicating an input error)
            }
            else
                return input.Any(char.IsUpper); // Check if str has any uppercase letters
        }

        /// <summary>
        /// Change name of Dictionary key
        /// </summary>
        /// <param name="dictionary">Target Dictionary to modify</param>
        /// <param name="keyOld">Name of old key</param>
        /// <param name="keyNew">Name of new key</param>
        /// <returns>Dictionary with modified key</returns>
        private SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)> ChangeDictionaryKey(SortedDictionary<string, (string Pronunciation, SpeechPartOfSpeech SpeechPart)> dictionary, string keyOld, string keyNew)
        {
            string pronunciation = dictionary[keyOld].Pronunciation; // Capture pronunciation
            SpeechPartOfSpeech speechPart = dictionary[keyOld].SpeechPart; // Capture speechPart
            dictionary.Remove(keyOld); // Remove old dictionary key entry
            dictionary[keyNew] = (pronunciation, speechPart); // Create new dictionary key entry
            return dictionary; // Return modified dictionary
        }

        /* /// <summary>
        /// Terminate DictionaryInterface processing
        /// </summary>
        public void TerminateInterface()
        {
            stopProcessing = true; // Set boolean flag
            cts.Cancel(); // Request cancellation of cancelable processes
        } */

        #endregion

    }
}