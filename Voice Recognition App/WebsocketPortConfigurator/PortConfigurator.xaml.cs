// Window application for configuring the websocket server port

using System;
using System.Text.RegularExpressions;
using System.Timers;
using System.Windows;
///using System.Windows.Controls;
using System.Windows.Media;

namespace WebsocketPortConfigurator
{
    /// <summary>
    /// Interaction logic for PortConfigurator.xaml
    /// </summary>
    public partial class PortConfigurator : Window
    {

        #region Fields

        Timer timer;

        #endregion

        #region Properties

        public int Port { get; private set; } = -1;

        #endregion

        #region Constructor

        public PortConfigurator(int port)
        {
            InitializeComponent();
            textboxPort.Text = port.ToString();
            textboxPort.Focus();
            textboxPort.SelectAll();
        }

        #endregion

        #region Event Methods

        /// <summary>
        /// Executes when cancel button is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        /// <summary>
        /// Executes when okay button is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OkayButton_Click(object sender, RoutedEventArgs e)
        {
            // Allow only numeric text input
            bool inputOkay;
            int port = -1;
            Regex regex = new Regex("[^0-9]+");
            var content = textboxPort.Text;
            if (regex.IsMatch(content) == true)
                inputOkay = false;
            else if (Int32.TryParse(content, out port) == false)
                inputOkay = false;
            else if (port < 1050 || port > 65500)
                inputOkay = false;
            else
                inputOkay = true;
            if (inputOkay == false)
            {
                if (timer != null) // Check if timer exists
                {
                    timer.Enabled = false; // Set timer as disabled
                    timer.Elapsed -= TimerElapsedEvent; // Unsubscribe from Elapsed event
                    timer.Dispose(); // Dispose timer
                    timer = null; // Set timer as null (cleanup)
                }
                timer = new System.Timers.Timer(); // Create new Timer instance
                timer.Interval = 1000; // Set timer countdown time
                timer.AutoReset = false; // Do not automatically restart timer
                timer.Elapsed += new ElapsedEventHandler(TimerElapsedEvent); // Subscribe MessageElapsedEvent method to Elapsed event
                this.Dispatcher.BeginInvoke(new Action(() => { // Invoke changing textboxPort foreground color and fontweight to red and bold, respectively
                    textboxPort.Foreground = Brushes.Red;
                    textboxPort.FontWeight = FontWeights.Bold;
                }));
                timer.Start(); // Start timer
            }
            else
            {
                this.Port = port;
                this.Close();
            }
        }

        /* /// <summary>
        /// Executes when textboxPort receives text input
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            // Allow only numeric text input
            var content = e.Text;
            Regex regex = new Regex("[^0-9]+");
            if (regex.IsMatch(content) == true)
                e.Handled = true;
            else if (Int32.TryParse(content, out int port) == false)
                e.Handled = true;
            else if (port < 2000 || port > 10000)
                e.Handled = true;
            else
                e.Handled = regex.IsMatch(content);
        } */

        /* /// <summary>
        /// Executes when content is pasted into textboxPort
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TextBox_Pasting(object sender, DataObjectPastingEventArgs e)
        {
            // Allow only numeric text input
            if (e.DataObject.GetDataPresent(typeof(String)))
            {
                string text = (string)e.DataObject.GetData(typeof(String));
                Regex regex = new Regex("[^0-9]+");
                if (regex.IsMatch(text) == true)
                    e.CancelCommand();
                else if (Int32.TryParse(text, out int port) == false)
                    e.Handled = true;
                else if (port < 2000 || port > 10000)
                    e.Handled = true;
            }
            else
                e.CancelCommand();
        } */

        /// <summary>
        /// Executes when timer expires
        /// </summary>
        /// <param name="source"></param>
        /// <param name="e"></param>
        private void TimerElapsedEvent(object source, ElapsedEventArgs e)
        {
            this.Dispatcher.BeginInvoke(new Action(() => { // Invoke changing textboxPort foreground color and fontweight back to black and regular, respectively
                textboxPort.Foreground = Brushes.Black;
                textboxPort.FontWeight = FontWeights.Regular;
            }));
            if (timer.Enabled == false) // Check if timer is NOT enabled
            {
                timer.Elapsed -= TimerElapsedEvent; // Unsubscribe from Elapsed event
                timer.Dispose(); // Dispose timer
                timer = null; // Set timer as null (cleanup)
            }
        }

        #endregion

    }
}
