#include "Server.hpp"

/******************************************************************************/
/*									CONNECTION PART							*/
/******************************************************************************/

/*Checks is user is connected*/
bool	Server::userIsConnected(User const &user) {
	return (!user.getUsername().empty() \
	&& !user.getNickname().empty() \
	&& user.isConnected());
}

/*Checks */
void	Server::checkConnection(User &user) {
	if (userIsConnected(user) && !user.isSent()) {
		welcome(user);
		user.setSent(true);
	}
}

/*Checks if password given by the user is right*/
bool	Server::password(User &user, std::vector<std::string> const &args) {
	if (user.isConnected())
		return (true);

	std::string	mess;

	mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Checking your password..."), std::string("AUTH"));
	sendMessageToUser(user, mess);
	if (args.size() != 1 || !checkPassword(args[0])) {
		mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Wrong password, please try again..."), std::string("AUTH"));
		sendMessageToUser(user, mess);
		removeUser(user, "");
		return (false);
	} else {
		mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Password is correct..."), std::string("AUTH"));
		sendMessageToUser(user, mess);
		user.setStatus(true);
		checkConnection(user);
		return (true);
	}
}

/*Checks the username and sets it*/
void	Server::userName(User &user, std::vector<std::string> const &args)
{
	std::string	mess;
	mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Checking your username..."), std::string("AUTH"));
	sendMessageToUser(user, mess);

	if (args.size() < 1) {
		mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Your username is empty..."), std::string("AUTH"));
		sendMessageToUser(user, mess);
		return ;

	} else {
		user.setUsername(args[0]);
		mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Username successfully set..."), std::string("AUTH"));
		sendMessageToUser(user, mess);
		checkConnection(user);
	}
}

/*Checks the nickname and sets it*/
void	Server::nickName(User &user, std::vector<std::string> const &args)
{
	std::string	mess;
	mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Checking your nickname..."), std::string("AUTH"));
	if (args.size() == 0 || isValidName(args[0]) == 0) {
		mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Your nickname is wrong..."), std::string("AUTH"));
		sendMessageToUser(user, mess);
		return ;
	}

	try {
		if (findUserByNickname(args[0])) {

			if (!user.isSent()) {
				std::string	err = ERR_NICKNAMEINUSE("*", args[0]);
				throw std::runtime_error(err);
			} else {
				std::string	err = ERR_NICKNAMEINUSE(user.getNickname(), args[0]);
				throw std::runtime_error(err);
			}
		} else {
			if (!user.isSent()) {
				user.setNickname(args[0]);
				mess = CMD_NICK(user.getSender(), args[0]);
				sendMessageToUser(user, mess);
			} else {
				mess = CMD_NICK(user.getSender(), args[0]);
				sendMessageToUser(user, mess);
				user.setNickname(args[0]);
			}
		}

		mess = CMD_NOTICE_TARGET(SERVER_NAME, std::string("*** Nickname successfully set..."), std::string("AUTH"));
		sendMessageToUser(user, mess);

		checkConnection(user);

	} catch(const std::exception& e) {
		std::cerr << e.what() << std::endl;
		sendMessageToUser(user, e.what());
	}
}

/*
Sends a formatted welcome message to
a newly connected user to the server.
*/
void    Server::welcome(User &user) const
{
	std::string message = _BOLD _MAGENTA"Welcome to our IRC server!\n";
	message += _MAGENTA"⠀⠀⠀⠀⣀⡤⢤⣄⠀⣠⡤⣤⡀⠀⠀⠀\n";
	message += _MAGENTA"⠀⠀⢀⣴⢫⠞⠛⠾⠺⠟⠛⢦⢻⣆⠀⠀\n";
	message += _MAGENTA"⠀⠀⣼⢇⣻⡀⠀⠀⠀⠀⠀⢸⡇⢿⣆⠀\n";
	message += _MAGENTA"⠀⢸⣯⢦⣽⣷⣄⡀⠀⢀⣴⣿⣳⣬⣿⠀\n";
	message += _MAGENTA"⢠⡞⢩⣿⠋⠙⠳⣽⢾⣯⠛⠙⢹⣯⠘⣷\n";
	message += _MAGENTA"⠀⠈⠛⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⠋⠁\n";
	message += _ITALIC _MAGENTA"                    Created by @mudoh, @lpradene and @madavid\n" _RESET;

	std::string mess;
	mess = RPL_WELCOME(user.getNickname(), message);
	sendMessageToUser(user, mess);

	mess = RPL_YOURID(user.getNickname());
	sendMessageToUser(user, mess);

	mess = RPL_YOURHOST(user.getNickname());
	sendMessageToUser(user, mess);

	mess = RPL_MYINFO(user.getNickname());
	sendMessageToUser(user, mess);

}

/*
Sends a formatted "PONG" response to a user,
typically used to respond to a "PING" message from the IRC server to keep the connection active. */
void	Server::pong(const User &user, std::vector<std::string> const & args) const
{
	if (args.size() < 1)
		return;
	std::string const & message = args[0];
	std::string mess = CMD_PING(message, user.getNickname());
	sendMessageToUser(user, mess);
}

/*Display informations about a user of the server*/
void	Server::whoIs(User &requestingUser, std::vector<std::string> const &args) const
{
	std::string	mess;

	if (args.size() < 1)
		return ;
	std::string const & investigatedUserNick = args[0];

	User 	*investigatedUser;
	try
	{
		investigatedUser = findUserByNickname(investigatedUserNick, requestingUser);
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(requestingUser, e.what());
		return ;
	}
	
	investigatedUser->whoIs(*this, requestingUser);
}

/*Format and send a message to the user to inform them about a new private message*/
void	Server::privmsg(const User &user, std::vector<std::string> const & args, std::string const &message) const
{
	if (args.size() < 1)
		return;
	std::string mess = CMD_PRIVMSG(user.getSender(), args[0], message);

	sendMessage(user, args[0], mess);
}

/*Sends a notice to the user*/
void	Server::notice(User &user, std::vector<std::string> const &	args)
{
	std::string	mess;
	User *userTarget;
	mess = CMD_NOTICE_TARGET(user.getSender(), mess, args[0]); // ???

	try
	{
		userTarget = findUserByNickname(args[0], user);
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(user, e.what());
		return ;
	}
	sendMessageToUser(*userTarget, mess);
}

/*Display informations about a user of the server*/
void	Server::userHost(User &user, std::vector<std::string> const &args)
{
	std::string		messtmp = "";

	std::string		mess;
	std::string 	infoTarget = "";
	if (args.size() == 0)
	{
		mess = RPL_USERHOST(user.getNickname(), infoTarget);
	}
	else
	{
		for (size_t i = 0; i < args.size(); i++)
		{
			User *userTarget;

			try
			{
				userTarget = findUserByNickname(args[i], user);
				std::string adress = inet_ntoa(userTarget->getAddr().sin_addr);
				infoTarget = args[i] + "=+~" + userTarget->getUsername() + "@" + adress;
				messtmp += infoTarget + " ";
			}
			catch(const std::exception& e){}
		}
		mess = RPL_USERHOST(user.getNickname(), messtmp);

	}

	sendMessageToUser(user, mess);
}

/******************************************************************************/
/*								   	CHANNEL COMMANDS									*/
/******************************************************************************/

/*Checks the parameters, creates the channels if it doesnt exists then tries to add the user to the channel*/
void	Server::joinChannel(User &user, std::vector<std::string> const &args)
{
	std::string channelName;
	std::string	password;
	std::string mess;
	Channel		*channel;
	bool 		op = false;
	std::string	chanBuffer;
	std::string	passBuffer;

	try {
		if (args.size() == 0 || args.size() > 2)
			throw needMoreParams(user.getNickname(), "JOIN");
	} catch(const std::exception& e) {
		sendMessageToUser(user, e.what());
		return ;
	}

	if (args.size() == 2)
		passBuffer = args[1];

	size_t	pos;
	chanBuffer = args[0];
	while (!chanBuffer.empty()) {
		pos = chanBuffer.find(',');
		if (pos ==  std::string::npos) {
			pos = chanBuffer.length();
			channelName = chanBuffer.substr(0, pos);
			chanBuffer.clear();

		} else {
			channelName = chanBuffer.substr(0, pos);
			chanBuffer = chanBuffer.substr(++pos, chanBuffer.length());
		}

		try {
			channel = findChannelByName(channelName, user);
		} catch(const std::exception& e) {
			channel = createChannel(user, channelName);
			op =  true;
		}

		if (args.size() == 2) {
			pos = passBuffer.find(',');
			if (pos ==  std::string::npos) {
				pos = passBuffer.length();
				password = passBuffer.substr(0, pos);
				passBuffer.clear();

			} else {
				password = passBuffer.substr(0, pos);
				passBuffer = passBuffer.substr(++pos, passBuffer.length());
			}
		}
		
		if (channel->addUser(*this, user, password, op))
			user.addChannel(channel);
	}
}

/*Checks the parameters,then tries to invite the user to the channel*/
void	Server::inviteChannel(User &invitingUser, std::vector<std::string> const & args)
{
	std::string	mess;

	try 
	{
		if (args.size() < 2)
			throw needMoreParams(invitingUser.getNickname(), "INVITE");
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(invitingUser, e.what());
		return ;
	}

	std::string const & invited = args[0];
	std::string const & channelName = args[1];
	
	Channel	*channel;
	try {
		channel = findChannelByName(channelName, invitingUser);
	} catch(const std::exception& e) {
		sendMessageToUser(invitingUser, e.what());
		return ;
	}

	User	*invitedUser;
	try
	{
		invitedUser = findUserByNickname(invited, invitingUser);
		channel->inviteUser(*this, *invitedUser, invitingUser);
		return ;
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(invitingUser, e.what());
		return ;
	}
}

/*Checks the parameters, then tries to remove the user from the channel*/
void	Server::partChannel(User &user,std::vector<std::string> const & args, std::string const & reason)
{
	std::string const	&channelName = args[0];
	Channel				*channel;
	std::string			mess;

	try 
	{
		if (args.size() < 1)
			throw needMoreParams(user.getNickname(), "PART");
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(user, e.what());
		return ;
	}

	try {
		channel = findChannelByName(channelName, user);
	} catch(const std::exception& e) {
		sendMessageToUser(user, e.what());
		return ;
	}

	if (channel->partUser(*this, user, reason))
	{
		user.leaveChannel(channel);
		if (channel->isEmpty())
			removeChannel(channel);
	}
}

/*Checks the parameters,then tries to kick the user to the channel*/
void	Server::kickChannel(User &kicker, std::vector<std::string> const & args, std::string const & reason) //peut etre merge avec part plus tard, mais la cest pour eviter les pb
{
	std::string	mess;

	try 
	{
		if (args.size() < 2)
			throw needMoreParams(kicker.getNickname(), "KICK");
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(kicker, e.what());
		return ;
	}

	std::string const &channelName = args[0];
	std::string const &kickedNick = args[1];
	
	Channel	*channel;
	try {
		channel = findChannelByName(channelName, kicker);
	} catch(const std::exception& e) {
		sendMessageToUser(kicker, e.what());
		return ;
	}

	User 	*kickedUser;
	try
	{
		kickedUser = findUserByNickname(kickedNick, kicker);
		if (channel->kickUser(*this, *kickedUser, kicker, reason))
		{
			kickedUser->leaveChannel(channel);
			if (channel->isEmpty())
				removeChannel(channel);
		}
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(kicker, e.what());
	}
	return ;
}

/*Checks the parameters,then tries to updates the channel topic*/
void	Server::topicChannel(User &user, std::vector<std::string> const & args, std::string const & topic)
{
	std::string	mess;
	
	try 
	{
		if (args.size() < 1)
			throw needMoreParams(user.getNickname(), "TOPIC");
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(user, e.what());
		return ;
	}


	std::string const & channelName = args[0];

	Channel	*channel;

	try {
		channel = findChannelByName(channelName, user);
	} catch(const std::exception& e) {
		sendMessageToUser(user, e.what());
		return ;
	}
	
	if (args.size() == 1) {
		mess = RPL_TOPIC(user.getNickname(), channel->getName(), channel->getTopic());
		sendMessageToUser(user, mess);

		mess = RPL_TOPICWHOTIME(user.getNickname(), channel->getName(), channel->getTopicUpdateUser(), channel->getTopicUpdateTimestamp());
		sendMessageToUser(user, mess);
	} else {
		channel->updateTopic(*this, user, topic);
	}
}

/*Checks the parameters,then tries to change channel modes*/
void	Server::modeChannel(User &user, std::vector<std::string> const &args)
{
	std::string	mess;
	try 
	{
		if (args.empty())
			throw needMoreParams(user.getNickname(), "MODE");
	}
	catch(const std::exception& e)
	{
		sendMessageToUser(user, e.what());
		return ;
	}

	const std::string &target = args[0];
	if (target[0] != '#') {
		return ;
	}

	Channel	*channel;
	
	try {
		channel = findChannelByName(target, user);
	} catch(const std::exception& e) {
		sendMessageToUser(user, e.what());
		return ;
	}

	channel->updateMode(*this, user, args);
}

/*Display informations about users of a given channel*/
void	Server::who(User &user, std::vector<std::string> const &args) const
{
	std::string	mess;
	
	if (args.size() < 1)
		return ;
	std::string const &channelName = args[0];

	//checks if channel exists
	Channel	*channel;
	try {
		channel = findChannelByName(channelName, user);
	
	} catch(const std::exception& e) {
		sendMessageToUser(user, e.what());
		return ;
	}
	
	channel->who(*this, user, true);
}