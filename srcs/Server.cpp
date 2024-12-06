#include "Server.hpp"
#include "Utils.hpp"
#include "User.hpp"

/******************************************************************************/
/*						CONSTRUCTORS & DESTRUCTORS							  */
/******************************************************************************/

/* Default constructor, we should make it private 
maybe to make sure we dont call it anywhere since
we're just using the parametrical one ? */
Server::Server(void) {}

/* Parametrical constructor :
- Initializes the _port and _password members with the provided values.
- Gets information about network addresses for server listening.
- Creates and binds a socket by looping through the results obtained.
*/
Server::Server(std::string const &  port, std::string const & password):
	_port(port),
	_password(password)
{
	struct addrinfo		hints;
	struct addrinfo		*res;
	struct addrinfo		*rp;
	int					status;

	// Define address info
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; // IPv4 and IPv6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; // For the exact IP address

	status = getaddrinfo(NULL, _port.c_str(), &hints, &res);
	if (status != 0) {
		throw std::runtime_error("Error: failed to connect to host");
	}

	int reuse = 1;

	for (rp = res; rp != NULL; rp = rp->ai_next) {
		_socketServer = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (_socketServer == -1)
			continue ;

		if (setsockopt(_socketServer, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
			freeaddrinfo(res);
			throw std::runtime_error("Error: cannot set option to socket");
		}

		if (fcntl(_socketServer, F_SETFL, O_NONBLOCK) == -1) {
			freeaddrinfo(res);
			throw std::runtime_error("Error: cannot set option to socket");
		}

		if (bind(_socketServer, rp->ai_addr, rp->ai_addrlen) == -1) {
			freeaddrinfo(res);
			throw std::runtime_error("Error: cannot bind");
		} else {
			freeaddrinfo(res);
			return ;
		}
	}
	
	throw std::runtime_error("Error: cannot bind");
}

/* Destructor, ensuring sever's socket is closed */
Server::~Server(void) {
	close(_socketServer);
	close(_epollfd);
}

/******************************************************************************/
/*						       EVENTS MANAGEMENT     						  */
/******************************************************************************/

/*
Initializes and runs the server,
handling events on file descriptors
using the epoll instance
and the handleEvents function.
*/
void	Server::run(void)
{
	struct epoll_event	ev;
	struct epoll_event	events[EVENTS_MAX];
	int					nfds;


	_epollfd = epoll_create1(0);
	if (_epollfd == -1) {
		throw std::runtime_error("Error: failed to create epoll");
	}

	if (listen(_socketServer, EVENTS_MAX) < 0) {
		throw std::runtime_error("Error: failed to listen on socket");
	}

	memset(&ev, 0, sizeof(ev));
	ev.events = EPOLLIN;
	ev.data.fd = _socketServer;
	if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, _socketServer, &ev) == -1) {
		throw std::runtime_error("Error: failed to manage sockets");
	}

	while (1) {
		nfds = epoll_wait(_epollfd, events, EVENTS_MAX, -1);
		if (g_end) {
			quit();
			return ;
		}

		if (nfds == -1) {
			throw std::runtime_error("Error: failed to received events");
		}
		
		for (int n = 0; n < nfds; ++n) {
			handleEvents(events[n].data.fd, events[n]);
		}
	}
}

/*
Handles events from epoll file descriptors,
processes data received from clients, 
manages user logins, 
and executes pending commands from logged in users.
*/
void	Server::handleEvents(int fd, struct epoll_event event)
{
	User	*user;

	if (fd == _socketServer) { // First connection of a client
		if (createUser())
			return ;

	} else { // Handle events from clients
		if (event.events != EPOLLIN)
			return ;

		user = findUserBySocket(fd);
		if (!user)
			return ;

		std::string	buffer = receiveData(fd);
		getCommands(*user, buffer);

		if (user->getCommands().size() >= 1) {
			execCommand(*user);
		}
	}
}

/*
Receives data from a socket and returns it as a string,
ensuring that the reception is complete up to the newline.
*/
std::string	Server::receiveData(int sockfd) const
{
    const int	bufferSize = 1024;
    char		buffer[bufferSize];
    ssize_t		bytes;
    std::string data;

    memset(buffer, 0, bufferSize);
    while (true) {
        bytes = recv(sockfd, buffer, bufferSize - 1, 0);
        if (bytes <= 0) {
            break ;
        }

        data += std::string(buffer, bytes);
        if (data[bytes - 1] != '\n') {
            break ;
        }
    }
    return (data);
}

/*
Parses commands received into a buffer by a user,
processes them one by one,
and updates the user's buffer accordingly.
_commandBuffer and _command of the user are set by setBuffer at eatch iteration
and each call fonction
*/
int	Server::getCommands(User &user, std::string & buffer) const
{
	size_t		pos = 0;

	buffer = user.getBuffer() + buffer;
	while (buffer.length()) {
		pos = buffer.find('\n');
		if (pos == std::string::npos) {
			user.setBuffer(buffer);
			return (1);
		}

		std::string	command = buffer.substr(0, pos);
		user.addCommand(command);
		buffer = buffer.substr(pos + 1, buffer.length());
		user.setBuffer(buffer);
	}
	return (0);
}

/*Parse the commands*/
void	Server::parseCommands(
	std::string &s,
	std::string &cmd,
	std::vector<std::string> &args,
	std::string &mess
) const {
	size_t  pos;

	pos = s.find('\r');
    if (pos != std::string::npos) {
        s = s.substr(0, pos);
	}

	pos = s.find(':');
    if (pos != std::string::npos) {
        mess = s.substr(++pos);
	}

    std::stringstream ss(s.substr(0, --pos));
    std::string token;

    std::getline(ss, cmd, ' ');
    while (std::getline(ss, token, ' ')) {
        args.push_back(token);
        token.clear();
    }
}

/*
Processes pending commands for a user
based on their type and content, performing
appropriate actions for each command type.
*/
void	Server::execCommand(User &user)
{
	std::deque<std::string>		commands = user.getCommands();

	while (commands.size())
	{
		std::string 				cmd;
		std::string 				mess;
		std::vector<std::string>	args;
		
		parseCommands(commands.front(), cmd, args, mess);
		commands.pop_front();

		if (cmd.empty())
			continue ;

		if (!userIsConnected(user)) {
			if (cmd == "PASS" && !password(user, args))
				return ;
			else if (cmd == "USER")
				userName(user, args);
			else if (cmd == "NICK")
				nickName(user, args);
			else if (cmd == "QUIT")
				return (removeUser(user, mess));
		} else {
			if (cmd == "NICK")
				nickName(user, args);
			else if (cmd == "PING")
				pong(user, args);
			else if (cmd == "NOTICE")
				notice(user, args);
			else if (cmd == "userhost")
				userHost(user, args);
			else if (cmd == "PRIVMSG")
				privmsg(user, args, mess);
			else if (cmd == "JOIN")
				joinChannel(user, args);
			else if (cmd == "PART")
				partChannel(user, args, mess);
			else if (cmd == "KICK")
				kickChannel(user, args, mess);
			else if (cmd == "INVITE")
				inviteChannel(user, args);
			else if (cmd == "TOPIC")
				topicChannel(user, args, mess);
			else if (cmd == "MODE")
				modeChannel(user, args);
			else if (cmd == "WHO")
				who(user, args);
			else if (cmd == "WHOIS")
				whoIs(user, args);
			else if (cmd == "QUIT")
				return (removeUser(user, mess));
		}
	}
	user.setCommands(commands);
}

void	Server::quit()
{
	for (std::vector<User *>::const_iterator it = _users.begin(); it != _users.end(); ++it) {
        delete *it;
    }

	for (std::vector<Channel *>::const_iterator it = _channels.begin(); it != _channels.end(); ++it) {
        delete *it;
    }
}

/******************************************************************************/
/*							USERS MANAGEMENT								  */
/******************************************************************************/

/*
Checks if the provided password matches the password stored in the Server object.
- Success:returns 1,
- Error: 0.
*/
int Server::checkPassword(std::string const & password) const {
	return (password == _password);
}


/*
- creates a new user, 
- creates a socket for it, 
- adds it to an epoll event monitoring mechanism,
- adds it to a list of connected users.
*/
//after create new user in our struct it's time to add him into the event instance
//whith epoll_ctl, we nedd a new struct epoll_event to associate with the sockfd user's
//we set ev.events with EPOLLIN so the instance will watch for this socket the EPOLLIN event only
//and we use EPOLL_CTL_ADD for adding the socket and the ev associate
int	Server::createUser()
{
	User				*user = new User();
	struct epoll_event	ev;
	int					sockfd;

	sockfd = user->createUserSocket(_socketServer);
	if (!sockfd) {
		delete user;
		return (1);
	}
	
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {
		delete user;
		return (1);
	}

	_users.push_back(user);
	return (0);
}

/*
Removes a user from the server by :
- removing its file descriptor from the epoll instance,
- removing it from the _users user vector,
- freeing the memory allocated for the user object.
*/
void	Server::removeUser(User &user, std::string const & reason)
{
	struct epoll_event				ev;
	std::vector<User *>::iterator	el;

	if (epoll_ctl(_epollfd, EPOLL_CTL_DEL, user.getSocket(), &ev) == -1)
		return ;

	el = std::find(_users.begin(), _users.end(), &user);
	if (el == _users.end())
		return ;
	_users.erase(el);
	
	user.quit(*this, reason);

	delete &user;
}

/*
Searches a list for a user based on the given socket file descriptor:
- success: returns the matching user if found,
- Error: returns NULL.
*/
User	*Server::findUserBySocket(const int sockfd) const
{
	for (std::vector<User *>::const_iterator it = _users.begin(); it != _users.end(); ++it) {
        if ((*it)->getSocket() == sockfd)
			return ((*it));
    }
	return (NULL);
}

/*
Searches for a user in a list based on the given nickname:
- success: returns the matching user if found,
- Error: throw an error.
*/
User	*Server::findUserByNickname(const std::string &targetNickname, User const &user) const
{
	for (std::vector<User *>::const_iterator it = _users.begin(); it != _users.end(); ++it) {
	    if ((*it)->getNickname() == targetNickname)
			return ((*it));
    }
	throw Server::noSuchNick(user.getNickname(), targetNickname);
}

/*
Searches for a user in a list based on the given nickname:
- success: returns the matching user if found,
- Error: returns NULL.
*/
User	*Server::findUserByNickname(const std::string &targetNickname) const
{
	for (std::vector<User *>::const_iterator it = _users.begin(); it != _users.end(); ++it) {
	    if ((*it)->getNickname() == targetNickname)
			return ((*it));
    }
	return (NULL);
}

/******************************************************************************/
/*							CHANNEL MANAGEMENT								  */
/******************************************************************************/

/*
Creates and adds a new channel to a list of channels.
Returns this newly created channel.
*/
Channel	*Server::createChannel(User &user, std::string const & channelName)//pas forcement besoin de *user 
{
	std::string			adress;
	std::string			creatorInfos;
	std::time_t			timestamp;
	std::stringstream	creationTime;
	Channel				*channel;

	adress = inet_ntoa(user.getAddr().sin_addr);
	creatorInfos = user.getNickname() + "!" + user.getUsername() + "@" + adress;
    timestamp = std::time(NULL);
	creationTime << timestamp;
	channel = new Channel(channelName, creatorInfos, creationTime.str());//a proteger avec une exception?

	_channels.push_back(channel);
	return (channel);
}

/*
Searches for a channel in a list based on the given name:
- success: returns the matching channel if found,
- Error: throw an exception.
*/
Channel	*Server::findChannelByName(std::string const & name, User const &user) const
{
	for (std::vector<Channel *>::const_iterator it = _channels.begin(); it != _channels.end(); ++it) {
		if ((*it)->getName() == name)
			return ((*it));
	}
	throw Server::noSuchChannel(user.getNickname(), name);
}

/*
Remove and delete the channel given as parameter.
*/
void	Server::removeChannel(Channel *channel)
{
	std::vector<Channel *>::iterator el;
	el = std::find(_channels.begin(), _channels.end(), channel);
	if (el == _channels.end())
		return ;
	
	_channels.erase(el);
	delete channel;
}

/******************************************************************************/
/*							MESSAGES MANAGEMENT								  */
/******************************************************************************/

/*
Sends a message to a user on its socket
- Success: returns 0
- Error: returns 1.
*/
int	Server::sendMessageToUser(const User &user, const std::string message) const
{
	std::cout << "sending to " + user.getNickname() + "... "  << message;
	int sendBytes = send(user.getSocket(), message.c_str(), message.length(), 0);
	if (sendBytes == -1)
		return (1);
	return (0);
}

/*
Sends a message to every member of a channel on their socket
- Success: returns 0
- Error: returns 1.
*/
void	Server::sendMessageToALL(const User &user, std::map<User *, bool> users, std::string message, bool toMe) const
{
	for (std::map<User *, bool>::iterator it = users.begin(); it != users.end(); it++) {
		if (!toMe && &user == it->first)
			continue ;
		sendMessageToUser(*(*it).first, message);
	}
}

/*
Sends a message to a user specified by nickname
or to all members of a channel specified by name,
depending on the given target.
*/
void	Server::sendMessage(const User &user, std::string target, std::string message) const
{
	Channel				*channel;
	User				*usertarget;
	
	try {
		channel = findChannelByName(target, user);
		if (channel->userOnChannel(const_cast<User &>(user)))
			sendMessageToALL(user, channel->getMembers(), message, false);
	
	} catch (const std::exception& e) {
		try {
			usertarget = findUserByNickname(target, user);
		
		} catch(const std::exception& e) {
			sendMessageToUser(user, e.what());
			return ;
		}

		sendMessageToUser(*usertarget, message);
	}
}
