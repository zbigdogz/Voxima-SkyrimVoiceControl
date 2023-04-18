using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.IO;

namespace Utilities
{
    public class Log
    {
        /* 
         * Add an "initialize logs" function that add the beginning information to all logs. Resets the log if needed, though I likely won't have that
         * 
         * 
         * 
         */

        #region structs/Enums
        public struct logs
        {
            public const string Activity = @"Data\SKSE\Plugins\VOX\Debug\Activity.txt";
            public const string Debug = @"Data\SKSE\Plugins\VOX\Logs\VoximaApp.log";
            public const string EnabledCommands = @"Data\SKSE\Plugins\VOX\Debug\Enabled Commands.txt";
            public const string AllCommands = @"Data\SKSE\Plugins\VOX\Debug\Commands.txt";
        }

        public enum LogType
        {
            Info,
            Error
        }

        #endregion

        static string originalActivity = File.ReadAllText(logs.Activity);
        static string originalDebug = File.ReadAllText(logs.Debug);
        static string newActivity = "";
        static string newDebug = "";

        public static void InitializeLogs()
        {
            string activityContent = File.ReadAllText(logs.Activity);
            string debugContent = File.ReadAllText(logs.Debug);

            if (originalActivity != "")
            {
                activityContent = "\n\n\n";
            }

            if (originalDebug != "")
            {
                debugContent = "\n\n\n";
            }

            activityContent += "Loaded Settings:\n--------------------";


            if (File.ReadAllText(logs.Debug) != "")
            {
                debugContent = "\n\n\n";
            }

            debugContent += "------------------------\n";
            debugContent += "|        Voxima        |\n";
            debugContent += "| Application Log File |\n";
            debugContent += "------------------------\n";

            originalActivity = File.ReadAllText(logs.Activity);

            Activity(activityContent, LogType.Info);
            Debug(debugContent, LogType.Info, false);
            EnabledCommands("", LogType.Info, true);
            AllCommands("", LogType.Info, true);
        }

        #region Functions for Different Logs

        /// <summary>
        /// Sends a log to Activity.txt
        /// </summary>
        public static void Activity(string logContent, LogType type = LogType.Info)
        {
            if (type == LogType.Error)
                logContent = $"\nERROR: {logContent}\n";

            newActivity += logContent + Environment.NewLine;

            FileWriteAsyncSafe(logs.Activity, newActivity, true, originalActivity); // Asynchronous-safe log file write
        }//End Log

        /// <summary>
        /// Sends a log to EnabledCommands.txt
        /// </summary>
        public static void EnabledCommands(string logContent, LogType type = LogType.Info, bool newFile = false)
        {
            if (type == LogType.Error)
                logContent = $"\nERROR: {logContent}\n";

            if (newFile)
                File.WriteAllText(logs.EnabledCommands, "");

            FileWriteAsyncSafe(logs.EnabledCommands, logContent + Environment.NewLine); // Asynchronous-safe log file write
        }//End Log

        /// <summary>
        /// Sends a log to Commands.txt
        /// </summary>
        public static void AllCommands(string logContent, LogType type = LogType.Info, bool newFile = false)
        {
            if (type == LogType.Error)
                logContent = $"\nERROR: {logContent}\n";

            if (newFile)
                File.WriteAllText(logs.AllCommands, "");

            FileWriteAsyncSafe(logs.AllCommands, logContent + Environment.NewLine); // Asynchronous-safe log file write
        }//End Log

        /// <summary>
        /// Sends a log to VoximaApp.log
        /// </summary>
        public static void Debug(string logContent, LogType type = LogType.Info, bool showInfo = true)
        {
            //Make the Error stick out when looking at the file
            if (type == LogType.Error)
                logContent = $"\t{logContent}";

            if (showInfo)
                logContent = $"{string.Format($"{DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss.fff")}")} [{type}] {logContent}";

            newDebug += logContent + Environment.NewLine;

            FileWriteAsyncSafe(logs.Debug, newDebug, true, originalDebug); // Asynchronous-safe log file write
        }//End Log

        /// <summary>
        /// Sends a log to Voxima's console window
        /// </summary>
        public static void Console(string logContent, LogType type = LogType.Info, bool showInfo = true)
        {
            //Make the Error stick out when looking at the file
            if (type == LogType.Error)
                logContent = $"\t{logContent}";

            if (showInfo)
                logContent = $"{string.Format($"{DateTime.Now.ToString("yyyy/MM/dd HH:mm:ss.fff")}")} [{type}] {logContent}";

            System.Console.WriteLine(logContent);
        }//End Log
        #endregion

        /// <summary>
        /// File writing that is tolerant to asynchronous scenarios
        /// </summary>
        /// <param name="content">Text content to write to file</param>
        static void FileWriteAsyncSafe(string address, string content, bool insertBefore = false, string oldLog = "")
        {
            try
            {
                locker.AcquireWriterLock(int.MaxValue); // Apply a file writer lock
                if (!insertBefore)
                    File.AppendAllText(address, content); // Perform synchronous file writing operation
                else
                    File.WriteAllText(address, content + oldLog); // Add "content", which is ALL content from this session to the top of the file
            }
            finally
            {
                locker.ReleaseWriterLock(); // Remove the file writer lock
            }
        }

        private static ReaderWriterLock locker = new ReaderWriterLock();


    }
}