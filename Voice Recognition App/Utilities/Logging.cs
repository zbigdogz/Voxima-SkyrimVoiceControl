// Class for performing logging activities

using System;
using System.IO;
using System.Threading;

namespace Utilities
{
    public class Logging
    {

        #region Fields

        private static string voxLogFilePath = @"Data\SKSE\Plugins\VOX\Logs";
        ///private static string skseLogFilePath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "My Games", "Skyrim Special Edition";
        private static LogOutput logOutput;

        #endregion

        #region Enumeration

        // Enumeration for different log output types
        public enum LogType
        {
            Debug,
            Info,
            Error
        }

        // Enumeration for destination of log content
        public enum LogOutput
        {
            Console,
            VOX_File,
            //SKSE_File
            //Plugin
        }

        #endregion

        #region Functional Methods

        /// <summary>
        /// Create and prepopulate speech recognition application log
        /// </summary>
        /// <returns>Boolean indicating if processing was successful</returns>
        public static bool InitializeLog(LogOutput outputType)
        {
            /* //This will give us the full name path of the assembly file:
            //i.e. C:\Program Files\MyApplication\MyApplication.exe
            string strExeFilePath = System.Reflection.Assembly.GetExecutingAssembly().Location;
            //This will strip just the working path name:
            //C:\Program Files\MyApplication
            string strWorkPath = System.IO.Path.GetDirectoryName(strExeFilePath);
            Console.WriteLine($"Assembly file path = {strExeFilePath}");
            Console.WriteLine($"Assembly directory path = {strWorkPath}"); */

            logOutput = outputType;

            try
            {
                // Set up logging path (if needed)
                if (Directory.Exists(voxLogFilePath) == false)
                {
                    var directoryInfo = Directory.CreateDirectory(voxLogFilePath);
                    /// *** Doesn't this mean the mod file structure is broken?
                }
                voxLogFilePath = Path.Combine(voxLogFilePath, "VoximaApp.log"); // Create path to log file

                // Create logging header and write it to the log file
                string message = null;
                //message += "-------------------------------------\n";
                //message += "| Voxima App Log File |\n";
                //message += "-------------------------------------\n";
                //message += "---------------------------------------------\n";
                //message += "| Voxima Application Log File |\n";
                //message += "---------------------------------------------\n";
                message += "------------------------\n";
                message += "|        Voxima        |\n";
                message += "| Application Log File |\n";
                message += "------------------------\n";
                if (logOutput == LogOutput.Console)
                    Console.WriteLine(message);
                else
                    File.WriteAllText(voxLogFilePath, message);
                WriteToLog("Application started and log file created", LogType.Info); // Write first entry to log file
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error while initializing log: {ex.Message}");
                Console.WriteLine("Press any key to continue");
                Console.ReadLine();
            }
            return false;
        }

        /// <summary>
        /// Write information to log file
        /// </summary>
        /// <param name="logContent">Text to log</param>
        /// <param name="type">Specific logging context</param>
        /// <param name="outputToConsole">Boolean indicating if log should be sent to application console</param>
        public static void WriteToLog(string logContent, LogType type)
        {
            try
            {
                logContent = $"{string.Format($"{DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss.fff")}")} [{type}] {logContent}"; // Modify the logContent to include time stamp and context info
                switch (logOutput)
                {
                    case LogOutput.Console:
                        Console.WriteLine(logContent);
                        break;
                    case LogOutput.VOX_File:
                        ///File.AppendAllText(logContent, logContent + Environment.NewLine); // Synchronous log file write (has issues with quick sequential write requests)
                        FileWriteAsyncSafe(logContent + Environment.NewLine); // Asynchronous-safe log file write
                        break;
                        //case LogOutput.SKSE_File:
                        //break;
                        //case LogOutput.Plugin:
                        //break;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error writing {logContent} to log: {ex.Message}");
                Console.WriteLine("Press any key to continue");
                Console.ReadLine();
            }
        }

        /// <summary>
        /// File writing that is tolerant to asynchronous scenarios
        /// </summary>
        /// <param name="content">Text content to write to file</param>
        public static void FileWriteAsyncSafe(string content)
        {
            try
            {
                locker.AcquireWriterLock(int.MaxValue); // Apply a file writer lock
                File.AppendAllText(voxLogFilePath, content); // Perform synchronous file writing operation
            }
            finally
            {
                locker.ReleaseWriterLock(); // Remove the file writer lock
            }
        }
        private static ReaderWriterLock locker = new ReaderWriterLock();

        #endregion

    }
}

#region Archive

/* /// <summary>
        /// Write information to log file
        /// </summary>
        /// <param name="Phrase">Text to log</param>
        public static void Log(string Phrase)
        {
            CurrentLog += Phrase + '\n';
            bool Success = false;

            while (!Success)
            {
                try
                {
                    System.IO.File.WriteAllText(ActivityDebugAddress, CurrentLog + '\n' + PreviousLog);
                    Success = true;
                }
                catch (Exception) { }
            }
        } */

//Console.WriteLine($"log location is {voxLogFilePath}");
//string strWorkPath = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
//Console.WriteLine($"exe location is {strWorkPath}");

//private static string activityLogPath = @"Plugins\VOX\Debug\ActivityLog.txt";
//private static string debugLogPath = @"Plugins\VOX\Debug\DebugLog.txt";
//private static string miscLogPath = @"Plugins\VOX\Debug\MiscLog.txt";

//private static async Task FileWriteAsync(string filePath, string messaage, bool append = true)
//{
//    using (FileStream stream = new FileStream(filePath, append ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.None, 4096, true))
//    using (StreamWriter sw = new StreamWriter(stream))
//    {
//        sw.WriteLineAsync(messaage);
//    }
//}

//private static void FileWriteAsync(string filePath, string messaage, bool append = true)
//{
//    Task.Run(async () =>
//    {
//        try
//        {
//            using (FileStream stream = new FileStream(filePath, append ? FileMode.Append : FileMode.Create, FileAccess.Write, FileShare.None, 4096, true))
//            using (StreamWriter sw = new StreamWriter(stream))
//            {
//                await sw.WriteLineAsync(messaage);
//            }
//        }
//        catch (Exception ex)
//        {
//            Console.WriteLine($"Error writing asynchronously to log file: {ex.Message}");
//        }
//    });
//}

//private static async Task WriteTextToFile(string file, List<string> lines, bool append)
//{
//    if (!append && File.Exists(file))
//        File.Delete(file);

//    using (var writer = File.OpenWrite(file))
//    {
//        using (var streamWriter = new StreamWriter(writer))
//            foreach (var line in lines)
//                await streamWriter.WriteLineAsync(line);
//    }
//}

#endregion