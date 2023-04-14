// Class for managing a websocket server

using System;
using System.Linq;
using System.Text;
using Utilities;
using WatsonWebsocket;

namespace WebSocket
{
    public class WebSocketServer
    {

        #region Fields

        WatsonWsServer _server;
        string _serverIp = null;
        int _serverPort = -1;
        Guid _clientGuid = Guid.Empty;

        #endregion

        #region Properties

        /// <summary>
        /// Indicates if server is listening for connections
        /// </summary>
        public bool IsListening { get { return _server.IsListening; } }

        /// <summary>
        /// Indicates if server is initialized
        /// </summary>
        public bool IsInitialized { get { return _server != null; } }

        /// <summary>
        /// Reports server's target IP address
        /// </summary>
        public string IP { get { return _serverIp; } }

        /// <summary>
        /// Reports server's target Port
        /// </summary>
        public int Port { get { return _serverPort; } }

        /// <summary>
        /// Reports server's connected client (only makes sense when only one client is connected)
        /// </summary>
        public Guid ClientGuid { get { return _clientGuid; } }

        /// <summary>
        /// Reports server's connection status with a single client
        /// </summary>
        public bool IsClientConnected
        {
            get
            {
                if (_clientGuid != Guid.Empty)
                    return _server.IsClientConnected(_clientGuid);
                else
                    return false;
            }
        }

        #endregion

        #region Constructor

        /// <summary>
        /// Constructor for WatsonWebSocketServer class
        /// </summary>
        /// <param name="serverIp"></param>
        /// <param name="serverPort"></param>
        /// <param name="ssl"></param>
        public WebSocketServer(string serverIp, int serverPort, bool ssl = false)
        {
            _serverIp = serverIp;
            _serverPort = serverPort;
            _server = new WatsonWsServer(serverIp, serverPort, ssl);
            _server.ClientConnected += ClientConnected;
            _server.ClientDisconnected += ClientDisconnected;
            _server.MessageReceived += MessageReceived;
            _server.ServerStopped += ServerStopped;
        }

        #endregion

        #region Event Methods

        /// <summary>
        /// Method run when websocket client connects to server
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>        
        private void ClientConnected(object sender, ConnectionEventArgs args)
        {
            //_serverIp = args.Client.Ip; /// Broken as of 1/8/23
            //_serverPort = args.Client.Port; /// Broken as of 1/8/23
            _clientGuid = args.Client.Guid;
            string argsClient = args.Client.ToString();
            ///Log.Debug($"Client connected via {_serverIp} on port {_serverPort} : {argsClient}", Log.LogType.Info);
            if (OnConnected != null) // Check that OnConnected is NOT null
                OnConnected(null, args); // Raise OnConnected event
        }

        /// <summary>
        /// Method run when websocket client disconnects from server
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void ClientDisconnected(object sender, DisconnectionEventArgs args)
        {
            Guid disconnectedClientGuid = args.Client.Guid;
            _clientGuid = Guid.Empty;
            ///Log.Debug($"Client disconnected: {disconnectedClientGuid}", Log.LogType.Info);
            if (OnDisconnected != null) // Check that OnDisconnected is NOT null
                OnDisconnected(null, args); // Raise OnDisconnected event
        }

        /// <summary>
        /// Method run when websocket server receives message from client
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void MessageReceived(object sender, MessageReceivedEventArgs args)
        {
            string message = Encoding.UTF8.GetString(args.Data.Array.TakeWhile(x => x != 0).ToArray()); // Convert received byte array into string and remove 0 values
            ///Log.Debug($"Received from client {args.Client.ToString()} : {message}", Log.LogType.Info);
            ExMessageReceivedEventArgs messageArgs = new ExMessageReceivedEventArgs(); // Create new ExMessageReceivedEventArgs instance
            messageArgs.Message = message; // Transfer message information
            if (OnMessageReceived != null) // Check that OnMessageReceived is NOT null
                OnMessageReceived(null, messageArgs); // Raise OnMessageReceived event
        }

        /// <summary>
        /// Method run when websocket server is stopped
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        private void ServerStopped(object sender, EventArgs args)
        {
            Log.Debug("Server stopped", Log.LogType.Info);
        }

        #endregion

        #region Functional Methods

        /// <summary>
        /// Method for starting websocket server
        /// </summary>
        public bool StartServer()
        {
            try
            {
                _server?.Start();
                Log.Debug("Application websocket server started", Log.LogType.Info);
                Log.Debug($"Server is listening {$"on port {this.Port}"}: {_server.IsListening}", Log.LogType.Info);
                return true;
            }
            catch (Exception ex)
            {
                if (ex.Message.Contains("conflicts with an existing registration"))
                    Log.Debug("Target websocket port is already being used by another process", Log.LogType.Info);
                else
                    Log.Debug($"Error starting server: {ex.Message}", Log.LogType.Info);
                return false;
            }
        }

        /// <summary>
        /// Method for stopping websocket server
        /// </summary>
        public void StopServer()
        {
            _server?.Stop();
            Log.Debug($"Server is listening: {_server.IsListening}", Log.LogType.Info);
        }

        /// <summary>
        /// Method for disposing of websocket server and removing its instance
        /// </summary>
        public void DisposeServer()
        {
            StopServer();
            _server?.Dispose();
            _server = null;
            Log.Debug("Server is disposed", Log.LogType.Info);
        }

        /// <summary>
        /// Method for sending (string) message from websocket client to websocket server
        /// </summary>
        /// <param name="message"></param>
        public void SendMessage(string message)
        {
            if (this.IsClientConnected == true)
            {
                if (_server?.SendAsync(_clientGuid, message).Result == true)
                    Log.Debug($"Sent to client: {message}", Log.LogType.Info);
                else
                    Log.Debug($"Server failed to send message: {message}", Log.LogType.Error);
            }
            else
                Log.Debug($"Cannot send message \"{message}.\" Server not connected to client.", Log.LogType.Error);
        }

        /// <summary>
        /// Method for disconnecting a specific websocket client
        /// </summary>
        /// <param name="guid"></param>        
        public void DisconnectClient(Guid guid)
        {
            _server?.DisconnectClient(guid);
            Log.Debug($"Server disconnected client with GUID {guid}", Log.LogType.Info);
        }

        /* /// <summary>
        /// Extract error code from exception
        /// </summary>
        /// <param name="ex">Exception of interest</param>
        /// <returns></returns>
        private int GetErrorCode(Exception ex)
        {
            var w32ex = ex as Win32Exception;
            if (w32ex == null)
                w32ex = ex.InnerException as Win32Exception;
            if (w32ex != null)
                return w32ex.ErrorCode;
            return -1;
        } */

        #endregion

        #region Event Handlers

        public event EventHandler<ExMessageReceivedEventArgs> OnMessageReceived; // Event to raise when server receives a message

        public event EventHandler OnConnected; // Event to raise when server connects to client

        public event EventHandler OnDisconnected; // Event to raise when server disconnects from client

        #endregion

        #region Event Arguments

        // Class for handling ProcessName data during ProcessClosed events
        public class ExMessageReceivedEventArgs : EventArgs
        {
            public string Message { get; set; }
        }

        #endregion

    }
}

#region Source Credit

// Websocket functionality provided via WatsonWebSocket by Joel Christner ==> https://github.com/jchristn/WatsonWebsocket

#endregion