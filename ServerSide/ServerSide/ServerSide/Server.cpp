#include "Server.h"

//Given constructor function
Server::Server()
{

	// this server use TCP. that why SOCK_STREAM & IPPROTO_TCP
	// if the server use UDP we will use: SOCK_DGRAM & IPPROTO_UDP
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (_serverSocket == INVALID_SOCKET)
		throw exception(__FUNCTION__ " - socket");
}

//Given destructor function
Server::~Server()
{
	try
	{
		// the only use of the destructor should be for freeing 
		// resources that was allocated in the constructor
		closesocket(_serverSocket);
	}
	catch (...) {}
}

//Given serve function
void Server::serve(int port)
{
	struct sockaddr_in sa = { 0 };

	sa.sin_port = htons(port); // port that server will listen for
	sa.sin_family = AF_INET;   // must be AF_INET
	sa.sin_addr.s_addr = INADDR_ANY;  // when there are few ip's for the machine. We will use always "INADDR_ANY"

	// Connects between the socket and the configuration (port and etc..)
	if (bind(_serverSocket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw exception(__FUNCTION__ " - bind");

	// Start listening for incoming requests of clients
	if (listen(_serverSocket, SOMAXCONN) == SOCKET_ERROR)
		throw exception(__FUNCTION__ " - listen");
	cout << "Listening on port " << port << endl;
}

//Given acceptClient function
void Server::acceptClientLoop()
{
	while (true)
	{
		// this accepts the client and create a specific socket from server to this client
		// the process will not continue until a client connects to the server
		SOCKET client_socket = accept(_serverSocket, NULL, NULL);
		if (client_socket == INVALID_SOCKET)
			throw std::exception(__FUNCTION__);

		cout << "Client accepted. Server and client can speak" << endl;
		cout << "Waiting for client connection request..." << endl;
		// the function that handle the conversation with the client
		thread ClientThread(&Server::clientHandler, ref(*this), ref(client_socket)); //creating a thread to handle each new client
		ClientThread.detach(); //detaching thread.
	}
}


/*
Function handles user login, and after that the type of update the user request's.
Input: SOCKET clientSocket
Output: none
*/
void Server::clientHandler(SOCKET clientSocket)
{
	//setting local function variables.
	string thisUser;
	string otherUser;
	string message;
	int nameLen = 0;
	int msgLen = 0;

	try
	{
		//checking if the Type Code of the user's socket requires a login.
		if (Helper::getMessageTypeCode(clientSocket) == MT_CLIENT_LOG_IN)
		{
			//getting the user's name length (2 bytes).
			nameLen = Helper::getIntPartFromSocket(clientSocket, 2);

			//getting the user's name (number of bytes depends on nameLen).
			thisUser = Helper::getStringPartFromSocket(clientSocket, nameLen);

			unique_lock<mutex> usersLock(_usersMutex); //locking the _usersOnline map.

			//adding the client's name and socket as a pair to the users map.
			_usersOnline.insert(make_pair(thisUser, clientSocket));

			usersLock.unlock(); //unlocking the _usersOnline map.

			//sending update message to the user with a string of all the connected users.
			Helper::send_update_message_to_client(clientSocket, NONE, NONE, allUsersStringByProtocol());
		}
		else
		{
			//if client's login process faild, throwing a login error exception.
			throw(exception("ERROR: LOGIN ERROR.\n"));
		}

		while (true) //infinite loop.
		{
			//checking if the Type Code of the user's socket requires an update message.
			if (Helper::getMessageTypeCode(clientSocket) == MT_CLIENT_UPDATE)
			{
				//getting the user's name length (2 bytes).
				nameLen = Helper::getIntPartFromSocket(clientSocket, 2);

				if (nameLen > 0) //if the user want's an update of some sort of a chat with someone.
				{
					//getting the other user's name (number of bytes depends on nameLen).
					otherUser = Helper::getStringPartFromSocket(clientSocket, nameLen);

					//getting the length of the message to send (5 bytes).
					msgLen = Helper::getIntPartFromSocket(clientSocket, 5);

					if (msgLen > 0) //if the user want's to send a message to someone.
					{
						//getting the message's content (number of bytes depends on msgLen).
						message = Helper::getStringPartFromSocket(clientSocket, msgLen);

						MessageStructure newMsg = {thisUser, otherUser, message}; //creating a new MessageStructure for the message.

						unique_lock<mutex> msgLock(_msgMutex); //locking the messages queue (_msgQueue).
						_msgQueue.push(newMsg); //pushing the created MessageStructure to the messages queue (_msgQueue).
						msgLock.unlock(); //unlocking the messages queue (_msgQueue).

						_msgCodVar.notify_one(); //notifing to one thread waiting that the _msgMutex is now unlocked.
					}
					else //if the client want's an update on a chat with someone without sending a message.
					{
						vector<string> namesVec = {thisUser, otherUser}; //creating a vector for the 2 names.
						sort(namesVec.begin(), namesVec.end()); //using sort (alphabetically)

						string path = "Chats_LOG/" + namesVec[0] + "&" + namesVec[1] + ".txt"; //building the path to the chat file based on the names.
						
						ifstream chatFile(path); //opening the file related to the current chat's path

						stringstream fileContent;
						fileContent << chatFile.rdbuf(); //reading the file's stream to a string stream variable (will be converted and sent as a regular string).

						//sending an update message to the user with all needed info. (according to the protocol).
						Helper::send_update_message_to_client(clientSocket, fileContent.str(), otherUser, allUsersStringByProtocol());
					}
				}
				else //if the client want's a regular update message.
				{
					//sending an update message to the user with all needed info. (according to the protocol).
					Helper::send_update_message_to_client(clientSocket, NONE, NONE, allUsersStringByProtocol());
				}
			}
		}
	}
	catch (const exception& e)
	{
		lock_guard<mutex> usersLock(_usersMutex); //locking the _usersOnline map to this scope.

		_usersOnline.erase(thisUser); //erasing the client's username and socket from the map.

		closesocket(clientSocket); //closing the client socket.
	}
}

/*
Function checks for messages in an infenate while(true) loop, and sends updates to the user according to the protocol.
Input: none
Output: none
*/
void Server::messagesHandler()
{
	//setting local function variables.
	stringstream fileContent;

	while (true) //infinite loop.
	{
		unique_lock<mutex> msgLock(_msgMutex); //locking the messages queue (_msgQueue).

		_msgCodVar.wait(msgLock, [&]{ return (!_msgQueue.empty());}); //waiting for _msgQueue to have at least one message.

		//getting the front message from _msgQueue and popping it. 
		MessageStructure msg = _msgQueue.front();

		_msgQueue.pop();

		msgLock.unlock(); //unlocking the messages queue (_msgQueue).

		vector<string> namesVec = { msg.sender, msg.receiver }; //creating a vector for the 2 names.
		sort(namesVec.begin(), namesVec.end()); //using sort (alphabetically)
		string path = "Chats_LOG/" + namesVec[0] + "&" + namesVec[1] + ".txt"; //building the path to the chat file based on the names.

		ofstream addToChatFile(path, ios_base::app); //opening the file related to the current chat's path for writing/creation. (ios_base::app - to keep current file data)

		if (!addToChatFile) //if file can't be created / does not exist
		{
			exit(1); //quiting the program.
		}

		addToChatFile << "&MAGSH_MESSAGE&&Author&" + msg.sender + "&DATA&" + msg.content; //writing the message to the file in the protocol.
		addToChatFile.close(); //closing the file.

		
		ifstream file(path); //opening the file related to the current chat's path
		fileContent << (file.rdbuf()); //reading the file's stream to a string stream variable (will be converted and sent as a regular string).

		//sending an update message to the user with all needed info. (according to the protocol).
		//(_usersOnline.find(m.sender)->second  -->  the socket of the user with this sender name from the map)
		cout << fileContent.str() + " , " + msg.receiver + " , " + allUsersStringByProtocol() << endl;
		Helper::send_update_message_to_client(_usersOnline.find(msg.receiver)->second, fileContent.str(), msg.sender, allUsersStringByProtocol());
	}
}

/*
Function returns a string of all the online users (names) in the requested protocol.
Input: none
Output: string usersStr
*/
string Server::allUsersStringByProtocol()
{
	string usersStr; //string variable to contain the result.

	unique_lock<mutex> usersLock(_usersMutex);//locking the _usersOnline map.

	//looping through the users map and adding all of the names to the string in the requested protocol.
	for (auto currentUser = _usersOnline.begin(); currentUser != _usersOnline.end(); ++currentUser)
	{
		usersStr += (currentUser->first + "&");
	}

	usersLock.unlock();//unlocking the _usersOnline map.

	return usersStr.substr(0, usersStr.length() - 1); //returning the result without the lase '&'.
}