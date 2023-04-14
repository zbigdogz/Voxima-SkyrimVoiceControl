// Websocket client setup, management, and message processing

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

using namespace std;

typedef websocketpp::client<websocketpp::config::asio_client> WebSocketClient;
typedef websocketpp::config::asio_client::message_type::ptr WebSocketMessagePtr;

constexpr auto WEBSOCKET_PORT = 19223;  // Default websocket port to target ///*** find way to set this via MCM
bool connected;
atomic_bool stopProcessing{false};
WebSocketClient _webSocketClient;
websocketpp::connection_hdl _serverConnectionHandle;
shared_ptr<websocketpp::connection<websocketpp::config::asio_client>> _clientConnection;

void ProcessReceivedMessage(const string& command);

// Send messages from client (SKSE plugin) to server (voice recognition app)
void SendMessage(const string& messageText) {
    try {
        if (connected)  // Check if client is currently connected to server
        {
            websocketpp::lib::error_code ec;
            _webSocketClient.send(_serverConnectionHandle, messageText, websocketpp::frame::opcode::text, ec);  // Send messageText to server

            if (messageText._Starts_with("update")) {
                std::string newMessage = "";

                for (int i = 0; i < messageText.length() && messageText[i] != '\r' && messageText[i] != '\n'; i++) {
                    newMessage += messageText[i];
                }
                logger::info("Sent message: {}", newMessage);

            } else
                logger::info("Sent message: {}", messageText);

            // if (ec)  // Check if error occurred while sending messageText
            //     logger::info("Error while sending {} message: {}", messageText, ec.message());
            // else
            //     logger::info("Sent message: {}", messageText);
        } else
            logger::error("Cannot send message \"{}.\" Client not connected.", messageText);
    } catch (exception ex) {
        logger::error("ERROR while sending \"{}\" message to client: {}", messageText, ex.what());
    }
}

// Terminate websocket client
void TerminateWebsocket() {
    try {
        logger::info("Terminating websocket client...");
        if (connected == true) {
            logger::info("Confirmed websocket must be terminated");
            /// SendMessage("KillServer");  // Send server message to trigger closing of speech recognition application
            websocketpp::lib::error_code ec;
            _webSocketClient.close(_clientConnection->get_handle(), websocketpp::close::status::normal, "Connection closed due to Skyrim exit", ec);  // Close active websocket connection
            if (ec) logger::error("ERROR while terminating connection: {}", ec.message());
        }
        stopProcessing = true;  // Set flag to (hopefully) trigger exit from threaded process (if applicable)
        logger::info("Websocket client terminated");
    } catch (exception ex) {
        logger::error("ERROR while terminating websocket client: {}", ex.what());
    }
}

// Launch companion speech recognition application
void LaunchSpeechRecoApp() {
    try {
        string path = "Data/SKSE/Plugins/DBU/Speech Recognition Application/Voxima.exe";  // Relative path to companion appliation that reflects the Skyrim SKSE plugin structure
        logger::info("Launching Voice Recognition from: {}", path);
        STARTUPINFO info = {sizeof(info)};
        PROCESS_INFORMATION processInfo;
        if (CreateProcessA(path.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, (LPSTARTUPINFOA)&info, &processInfo))  // Create new process to launch target application and check if successful
        {
            /// logger::info("Speech recognition application opened");
            ///  WaitForSingleObject(processInfo.hProcess, INFINITE);
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
            logger::info("Voice recognition application launched");
        } else
            logger::error("Could not open speech recognition application at \"{}\"", path);
    } catch (exception ex) {
        logger::error("ERROR launching speech recognition application from plugin: {}", ex.what());
    }
}

// Write websocket port configuration to file
void ConfigureWebsocketPort() {
    try {
        /// string portConfigPath = "Data\\SKSE\\Plugins\\DBU\\Websocket Configuration\\PortConfig.txt";
        string portConfigPath = "Data/SKSE/Plugins/DBU/Websocket Configuration/PortConfig.txt";
        ofstream filePortConfig;
        filePortConfig.open(portConfigPath);
        filePortConfig.clear();
        filePortConfig << 19223;
        filePortConfig.close();
        logger::info("Websocket port config file created");
    } catch (exception ex) {
        logger::error("ERROR while writing websocket port configuration to file: {}", ex.what());
    }
}

// Set up websocket client within SKSE plugin
void InitializeWebSocketClient() {
    logger::info("Initializing websocket client");
    if (stopProcessing == true) {
        logger::info("Call to exit");
        exit;
    }
    try {
        _webSocketClient.clear_access_channels(websocketpp::log::alevel::all);
        _webSocketClient.clear_error_channels(websocketpp::log::alevel::all);
        _webSocketClient.set_open_handler([](const websocketpp::connection_hdl& connection) {  // Set up handler and associated actions to run when client successfully connects with server
            logger::info("Websocket connected");
            _serverConnectionHandle = connection;
            connected = true;
            atexit(TerminateWebsocket);  // Set up TerminateWebsocket method to run when Skyrim exits
        });
        _webSocketClient.set_close_handler(websocketpp::lib::bind(
            [](WebSocketClient* c,
               websocketpp::connection_hdl hdl) {  // Set up handler and associated actions to run when client's connection with server is closed
                logger::info("Websocket client connection with server closed");
                connected = false;
                InitializeWebSocketClient();
            },
            &_webSocketClient, ::websocketpp::lib::placeholders::_1));
        _webSocketClient.set_fail_handler(websocketpp::lib::bind(
            [](WebSocketClient* c,
               websocketpp::connection_hdl hdl) {  // Set up handler and associated actions to run when client fails to connect with server
                connected = false;
                auto errorMessage = _clientConnection->get_ec().message();
                logger::error("Websocket client connection failed: {}", errorMessage);
                ///_webSocketClient.stop();  // Stops the _webSocketClient
                ///_webSocketClient.interrupt(_clientConnection->get_handle());
                ///_webSocketClient.reset();
                InitializeWebSocketClient();
            },
            &_webSocketClient, ::websocketpp::lib::placeholders::_1));
        _webSocketClient.set_message_handler(
            [](const websocketpp::connection_hdl&, const WebSocketMessagePtr& message) {  // Set up handler for messages received from server
                ProcessReceivedMessage(message->get_payload());                           // Process the message received from server
            });
    } catch (exception ex) {
        logger::error("Error while setting up websocket client handlers: {}", ex.what());
    }

    try {
        bool getThread = false;
        if (_clientConnection == NULL) {
            logger::info("Initializing client asio");
            _webSocketClient.init_asio();  // Initialize websocket client
            getThread = true;
        }
        // else
        //{
        //     logger::info("sleeping");
        //     Sleep(2000);
        //     logger::info("done sleeping");
        // }

        auto uri = format("ws://localhost:{}", WEBSOCKET_PORT);  // Specify the target websocket host and port
        websocketpp::lib::error_code errorCode;
        _clientConnection = _webSocketClient.get_connection(uri, errorCode);  // Create connection configuration for _webSocketClient based on target uri
        _webSocketClient.connect(_clientConnection); // Attempt to establish a client connection
        if (getThread == true) {
            logger::info("Starting client run thread");
            thread([]() {
                _webSocketClient.run();  // Run _webSocketClient processing
                logger::debug("run done!");
            }).detach();  // Run processing of _webSocketClient in separate thread
        }
        logger::info("Websocket client initialization complete");
        if (connected == false) {
            thread([]() {
                logger::info("Start running connection timer");
                Sleep(2000);  // Pause to allow websocket connection to be made
                if (connected == false) {
                    logger::error("Websocket connection not established!");
                    /// RE::DebugNotification("Websocket connection not established!");
                }
            }).detach();  // Run checking of _webSocketClient connection status in separate thread
        }
    } catch (exception ex) {
        logger::error("ERROR while finishing initialization of websocket client: {}", ex.what());
    }
}