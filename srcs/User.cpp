#include "User.hpp"
#include "Channel.hpp"
#include "Utils.hpp"

/******************************************************************************/
/*					CONSTRUCTORS, DESTRUCTORS & OPERATORS					  */
/******************************************************************************/

/*
Default constructor seting up a User object
with default values for its member variables.
*/
User::User(void) :
	_username(""),
	_nickname(""),
	_inetNtoa(""),
	_sender(""),
	_isConnected(false),
	_connectionSent(false)
{}

/*
Constructor allowing the creation of a User object
with specified username and nickname values,
initializing the connection status to false by default.
*/
User::User(std::string const & username, std::string const & nickname) : 
	_username(username),
	_nickname(nickname),
	_inetNtoa(""),
	_sender(""),
	_isConnected(false),
	_connectionSent(false)
{}

/*
Destructor ensureing that the socket
associated with a User object is properly
closed when the object is destroyed.
*/
User::~User(void) { close(_socket); }

/******************************************************************************/
/*							SETTERS,  GETTERS AND UPDATERS						  */
/******************************************************************************/

void	User::setUsername(std::string const & username) { _username = username; }
void	User::setNickname(std::string const & nickname) {  _nickname = nickname; updateSender();}
void	User::setCommands(std::deque<std::string> const & commands) { _commands = commands; }
void	User::setBuffer(std::string const & buffer) { _commandBuffer = buffer; }
void	User::setAddr(sockaddr_in const & addr) { _addr = addr; }
void	User::setSocket(int const & socket) { _socket = socket; }
void	User::setInet(std::string const & inet) { _inetNtoa = inet; }
void	User::setStatus(bool const & connected) { _isConnected = connected; }
void	User::setSent(bool const & connectionSent) { _connectionSent = connectionSent; }

const std::string& 				User::getUsername() const { return (_username); }
const std::string&				User::getNickname() const { return (_nickname); }
const std::string&				User::getInet() const { return (_inetNtoa); }
const int&						User::getSocket() const { return (_socket); }
const struct sockaddr_in&		User::getAddr() const { return (_addr); }
const std::string&				User::getBuffer() const { return (_commandBuffer); }
const std::deque<std::string>&	User::getCommands() const { return (_commands); }
const std::string&				User::getSender() const  {return (_sender); }
const std::string				User::getChannelJoined() const {
	std::string channelJoinedStr;
	for (size_t i = 0; i < _channelsJoined.size(); ++i)	{
		channelJoinedStr.append(_channelsJoined[i]->getName() + " ");
	}
	return (channelJoinedStr);
}

void	User::updateSender() {
	_sender.clear();
	std::string adress = inet_ntoa(_addr.sin_addr);
	_sender = ":" + _nickname + "!" + "~" + _username + "@" + adress;
}

/*
Allows adding commands to the list 
of commands associated with a User object.
Each command added using this function is
stored in the _commands vector for later processing.
*/
void	User::addCommand(std::string const & command) { _commands.push_back(command); }

/******************************************************************************/
/*									CONNECTIONS									*/
/******************************************************************************/

/*
Accepts an incoming connection on a server socket,
configures the socket of the new connection in non-blocking mode,
updates the client information in a user object,
returns the file descriptor of the new socket.
If an error occurs at any step, the function returns 0.
*/
int	User::createUserSocket(int socketServer)
{
	int			socket;
	sockaddr_in	addr;
	socklen_t	size;

	size = sizeof(addr);
	socket = accept(socketServer, (struct sockaddr *)&addr, &size);
	if (socket == -1)
		return (0);

	if (fcntl(socket, F_SETFL, O_NONBLOCK) == -1)
		return (0);

	setAddr(addr);
	setSocket(socket);
	setInet(inet_ntoa(addr.sin_addr));
	return (socket);
}

void	User::quit(Server &server, std::string const & reason)
{
	Channel* channel;
	for (size_t i = 0; i < _channelsJoined.size(); i++)
	{
		channel = _channelsJoined[i];
		channel->quit(server, *this, reason);
		if (channel->isEmpty())
			server.removeChannel(channel);
	}
}

const bool&						User::isConnected() const { return (_isConnected); }
const bool&						User::isSent() const { return (_connectionSent); }

/******************************************************************************/
/*								CHANNEL MANAGEMENT							  */
/******************************************************************************/

void	User::addChannel(Channel *channel)
{	
    std::vector<Channel*>::iterator it = std::find(_channelsJoined.begin(), _channelsJoined.end(), channel);

	if (it == _channelsJoined.end()) {
		_channelsJoined.push_back(channel);
	}
}

void	User::leaveChannel(Channel *channel)
{

    std::vector<Channel*>::iterator it = std::find(_channelsJoined.begin(), _channelsJoined.end(), channel);

    if (it == _channelsJoined.end()) {
        return ; // ce cas devrait jamais se produire en vrai
    } else {
        _channelsJoined.erase(it);
    }

}

/******************************************************************************/
/*										USER INFO 								  */
/******************************************************************************/

void	User::whoIs(Server const &server, User &requestingUser) const
{
	std::string mess;

	mess = RPL_WHOISUSER(requestingUser.getNickname(), _nickname, "[id]", "[realhost]", "[realname]");
	server.sendMessageToUser(requestingUser, mess);

	mess = RPL_WHOISSERVER(requestingUser.getNickname(), _nickname);
	server.sendMessageToUser(requestingUser, mess);

	mess = RPL_WHOISCHANNELS(requestingUser.getNickname(), _nickname, getChannelJoined());
	server.sendMessageToUser(requestingUser, mess);

	mess = RPL_ENDOFWHOIS(requestingUser.getNickname(), _nickname);
	server.sendMessageToUser(requestingUser, mess);
}
