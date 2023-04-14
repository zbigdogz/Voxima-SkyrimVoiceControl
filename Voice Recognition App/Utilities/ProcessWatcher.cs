// Class for monitoring and reacting to external process activity

using System;
using System.Diagnostics;
using System.Threading;
using System.Management;

namespace Utilities
{
    public static class ProcessWatcher
    {

        #region Fields

        private static ManagementEventWatcher startWatch;
        private static string exitProcessName;
        private static Thread thread;

        #endregion

        #region Event Handlers

        public static event EventHandler<ProcessLaunchedEventArgs> ProcessLaunched; // Event to raise when target process launches
        public static event EventHandler<ProcessClosedEventArgs> ProcessClosed; // Event to raise when target process exits

        #endregion

        #region Functional Methods

        /// <summary>
        /// Monitor process for exit. Target process must be already running.
        /// </summary>
        /// <param name="processNames">Names of processes to monitor (first process found will be monitored)</param>
        /// <returns>Boolean indicating if target processes were found and exit monitoring is active</returns>
        public static bool MonitorForExit(string[] processNames)
        {
            try // Attempt the following code...
            {
                foreach (var processName in processNames) // Loop through all processNames
                {
                    Process[] processes = Process.GetProcessesByName(processName); // Obtain Process associated with inputted processName
                    if (processes.Length > 0) // Check if any processes were obtained
                    {
                        exitProcessName = processes[0].ProcessName; // Store name of process to target for exit monitoring
                        ExitMonitorInitialize(processes[0]); // Call method to monitor first index within processes for exit
                        Log.Debug($"Monitoring \"{processName}\" process for closing", Log.LogType.Info);
                        return true; // Return true indicating exit monitoring is active
                    }
                }
                Log.Debug($"Target {(processNames.Length > 0 ? $"processes" : "process")} \"{string.Join(", ", processNames)}\" not running", Log.LogType.Info);
                return false; // Return false indicating unsuccessful processing (no target processes were found)
            }
            catch (Exception ex) // Handle exceptions enountered in above code
            {
                Log.Debug($"Error encountered while setting up process exit monitor: {ex.Message}", Log.LogType.Error);
                return false; // Return false indicating unsuccessful processing (error encountered)
            }
        }

        /// <summary>
        /// Monitor process for launch. Works regardless of target process activity state.
        /// </summary>
        /// <param name="processName">Names of process to monitor</param>
        /// <returns>Boolean indicating if target processes were found and launch monitoring is active</returns>
        public static bool MonitorForStart(string processName)
        {
            try // Attempt the following code...
            {
                if (processName.EndsWith(".exe") == false) // Check if inputted processName ends with ".exe"
                    processName += ".exe"; // Append ".exe" if needed
                string responseRate = "2"; // Set maximum time for responding to process start events [seconds]
                string queryString =  // Build queryString for process watching
                    "SELECT *" +
                    "  FROM __InstanceCreationEvent " +
                    "WITHIN  " + responseRate +
                    " WHERE TargetInstance ISA 'Win32_Process' " +
                    "   AND TargetInstance.Name = '" + processName + "'";

                startWatch = new ManagementEventWatcher(queryString); // Create new ManagementEventWatcher instance for watching process per queryString
                startWatch.EventArrived += OnProcessLaunched; // Subscribe OnProcessLaunched to EventArrived events
                startWatch.Start(); // Start the WMI process launch event monitor (watcher)
                return true; // Return true indicating launch monitoring is active
            }
            catch (Exception ex) // Handle exceptions enountered in above code
            {
                /// Log($"Error encountered while setting up process launch monitor: {ex.Message}"); // Output info to event log /// *** need to have logging in a separate project for access by all projects
                return false; // Return false indicating unsuccessful processing (error encountered)
            }
        }

        /// <summary>
        /// Set up exit process exit monitoring
        /// </summary>
        /// <param name="process">Process to monitor for exit</param>
        private static void ExitMonitorInitialize(Process process)
        {
            thread = new Thread(() => // Create new thread instance
            {
                process.WaitForExit(); // Wait indefinitely until process exits
                ProcessClosedEventArgs args = new ProcessClosedEventArgs(); // Create new ProcessClosedEventArgs instance
                args.ProcessName = exitProcessName; // Transfer exitProcessName information
                OnProcessClosed(args); // Call method to raise ProcessClosed event
                exitProcessName = null; // Set exitProcessName as null for cleanup
                args = null; // Set args as null for cleanup
            });
            thread.Start(); // Start thread
        }

        /// <summary>
        /// Terminate all process watching
        /// </summary>
        public static void TerminateProcessWatcher()
        {
            if (thread != null) // Check if thread is NOT null
            {
                if (thread.IsAlive == true) // Check if thread is active (alive)
                    thread.Abort(); // Abort thread
                thread = null; // Set thread as null for cleanup
            }
            if (startWatch != null) // Check if startWatch is NOT null
            {
                startWatch.Stop(); // Stop the process watcher
                startWatch.Dispose(); // Dispose the process watcher
                startWatch = null; // Set startWatch as null for cleanup
            }
            exitProcessName = null; // Set exitProcessName as null for cleanup
        }

        #endregion

        #region Event Triggers

        /// <summary>
        /// Raises ProcessLaunched event via WMI methods
        /// </summary>
        /// <param name="e"></param>
        private static void OnProcessLaunched(object sender, EventArrivedEventArgs e)
        {
            try // Attempt the following code...
            {
                startWatch.Stop(); // Stop the WMI process launch event monitor (watcher)
                ///Log($"Process stopped: {e.NewEvent.Properties["ProcessName"].Value}"); // Output info to event log (debug)
                ProcessLaunchedEventArgs args = new ProcessLaunchedEventArgs(); // Create new ProcessLaunchedEventArgs instance
                args.ProcessName = e.NewEvent.Properties["ProcessName"].Value.ToString(); // Transfer launch event's target process information
                if (ProcessLaunched != null) // Check that ProcessLaunched is NOT null
                    ProcessLaunched(null, args); // Raise ProcessLaunched event
                args = null; // Set args as null for cleanup
            }
            finally // Perform after try
            {
                startWatch.Dispose(); // Dispose of startWatch
                startWatch = null; // Set startWatch as null for cleanup
            }
        }

        /// <summary>
        /// Raises ProcessClosed event via threaded WaitForExit
        /// </summary>
        /// <param name="e"></param>
        private static void OnProcessClosed(ProcessClosedEventArgs e)
        {
            if (ProcessClosed != null) // Check that ProcessClosed is NOT null
                ProcessClosed(null, e); // Raise ProcessClosed event
        }

        #endregion

        #region Event Arguments

        // Class for handling ProcessName data during ProcessClosed events
        public class ProcessClosedEventArgs : EventArgs
        {
            public string ProcessName { get; set; }
        }

        // Class for handling ProcessName data during ProcessLaunched events
        public class ProcessLaunchedEventArgs : EventArgs
        {
            public string ProcessName { get; set; }
        }

        #endregion

    }
}

#region Process Start Monitoring Example

/* string targetProcess = "skyrimSE.exe"; // Specify process to monitor for launch
if (ProcessWatcher.MonitorForStart(targetProcess) == true) // Call method to monitor the targetProcess for launch (via WMI) and check if processing was successful
{
    ProcessWatcher.ProcessLaunched += OnProcessLaunched; // Subscribe OnProcessLaunched to ProcessLaunched events
}

/// <summary>
/// Event when a target process is launched
/// </summary>
/// <param name="sender"></param>
/// <param name="e"></param>
private static void OnProcessLaunched(object sender, ProcessLaunchedEventArgs e)
{
    Log($"Target process {e.ProcessName} closed"); // Output info to event log (debug) /// *** need to have logging in a separate project for access by all projects

    /// *** now do stuff as a result of the target process being launched
} */

#endregion

#region Alternative Process Exit Monitoring Example

//// Set up the event handler for the exit monitoring "ProcessClosed" event
//ProcessWatcher.ProcessClosed += new EventHandler((s, e) =>
//{
//    Log("Target process closed"); // Output info to event log /// *** need to have logging in a separate project for access by all projects
//    /// *** now do stuff as a result of the target process being closed
//});

#endregion

#region References

// WaitForExit close watching ==> https://stackoverflow.com/questions/4119069/watch-another-application-and-if-it-closes-close-my-app-without-polling-c-shar
// WMI process launch watching ==> https://stackoverflow.com/questions/1986249/net-process-monitor/1986856#1986856
// Deep dive WMI event watching ==> https://www.codeproject.com/Articles/12138/Process-Information-and-Notifications-using-WMI
// Event consumption ==> https://learn.microsoft.com/en-us/dotnet/standard/events/how-to-raise-and-consume-events
#endregion