#ifndef _SERVER_HPP
# define _SERVER_HPP

# include <iostream>
# include <sstream>
# include <stdlib.h>
# include <stdio.h>
# include <unistd.h>
# include <cerrno>
# include <string>
# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <fcntl.h>
# include <sys/epoll.h>
# include <vector>
# include <map>
# include <algorithm>
# include <ctime>
#include <signal.h>

# include "Format.hpp"
# include "User.hpp"
# include "Channel.hpp"

# define DISPLAY 0
# define MODIFY 1
# define EVENTS_MAX 8
# define BUFFER_SIZE 4096
# define SERVER_NAME ":irc.serv.M.M.L "
# define SERVER_DESCRIPTION "very cool server"

# define CMD_PRIVMSG(sender, target, message)						((std::string)sender + " PRIVMSG "  + (target.empty() ? ":" : target + " :") + message + "\r\n");
# define CMD_NICK(sender, newNick)									((std::string)sender + " NICK :" + newNick + "\r\n");
# define CMD_NOTICE_TARGET(sender, message, target)					((std::string)sender + " NOTICE " + (target.empty() ? "" : target + " ") + message + "\r\n");
# define CMD_PING(target, targetNick)								((std::string)SERVER_NAME + "PONG " + target + " :" + targetNick + "\r\n");

# define RPL_WELCOME(nickName, message)								((std::string)SERVER_NAME + "001 " + nickName + " :" + message + "\r\n");
# define RPL_YOURID(nickName)										((std::string)SERVER_NAME + "002 " + nickName + " :" + "Your host is " + SERVER_NAME + ", running version irc-1.0" + "\r\n");
# define RPL_YOURHOST(nickName)										((std::string)SERVER_NAME + "003 " + nickName + " :" + "This server was created Tue Mars 23 2024 at 22:15:05 CEST" + "\r\n");
# define RPL_MYINFO(nickName)										((std::string)SERVER_NAME + "004 " + nickName + " " + SERVER_NAME + "\r\n");
# define RPL_USERHOST(nickName, infoTarget)							((std::string)SERVER_NAME + "302 " + nickName + " :" + infoTarget + "\r\n");

# define ERR_NOSUCHNICK(nickName, attemptedTarget)					((std::string)SERVER_NAME + "401 " + nickName + " " + attemptedTarget + " :No such nick/channel" + "\r\n");
# define ERR_NOSUCHCHAN(nickName, attemptedTarget)					((std::string)SERVER_NAME + "403 " + nickName + " " + attemptedTarget + " :No such channel" + "\r\n");
# define ERR_NICKNAMEINUSE(userCurrentNick, attemptedNick)			((std::string)SERVER_NAME + "433 " + userCurrentNick + " " + attemptedNick + " :Nickname is already in use." + "\r\n");
# define ERR_NEEDMOREPARAMS(nickName, command)						((std::string)SERVER_NAME + "461 " + nickName + " " + command + " :Not enough parameters" + "\r\n");

class User;
class Channel;
class Bot;

extern bool	g_end;

class Server {

public:

	//CONSTRUCTORS & DESTRUCTORS
	Server(std::string const &port, std::string const &password);
	~Server(void);

	//EVENTS AND COMMANDS MANAGEMENT
	void		run();
	void		handleEvents(int fd, struct epoll_event event);
	std::string	receiveData(int sockfd) const;
	int			getCommands(User &user, std::string & buffer) const;
	void		parseCommands(std::string &s, std::string &cmd, std::vector<std::string> &args, std::string & mess) const;
	void		execCommand(User &user);
	void		quit();

	//USERS MANAGEMENT
	int		checkPassword(std::string const &password) const;
	int		createUser();
	void	removeUser(User &user, std::string const &reason);
	User	*findUserBySocket(const int sockfd) const;
	User	*findUserByNickname(const std::string &targetNickname, User const &user) const;
	User	*findUserByNickname(const std::string &targetNickname) const;

	//CHANNEL MANAGEMENT
	Channel	*createChannel(User &user, std::string const &channelName);
	Channel	*findChannelByName(std::string const &name, User const &user) const;
	void	removeChannel(Channel *channel);

	//MESSAGES MANAGEMENT
	int			sendMessageToUser(const User &user, const std::string message) const;
	void		sendMessageToALL(const User &user, std::map<User *, bool> users, std::string message, bool ToMe = true) const;
	void		sendMessage(const User &user, const std::string target, const std::string message) const;
	

	//COMMANDS
		//USER AND CONNECTION RELATED COMMANDS
		bool	userIsConnected(User const &user);
		void	checkConnection(User &user);
		bool	password(User &user, std::vector<std::string> const &args);
		void	userName(User &user, std::vector<std::string> const &args);
		void	nickName(User &user, std::vector<std::string> const &args);
		void	welcome(User &user) const;
		void    pong(const User &user, std::vector<std::string> const &args) const;
		void	whoIs(User &requestingUser, std::vector<std::string> const &args) const;
		void	privmsg(const User &user, std::vector<std::string> const &args, std::string const &message) const;
		void	notice(User &user, std::vector<std::string> const &args);
		void	userHost(User &user, std::vector<std::string> const &args);
		
		//CHANNEL COMMANDS
		void	joinChannel(User &user, std::vector<std::string> const &args);
		void	inviteChannel(User &invitingUser, std::vector<std::string> const &args);
		void	partChannel(User &user, std::vector<std::string> const &args, std::string const &reason);
		void	kickChannel(User &kicker, std::vector<std::string> const &args, std::string const &reason);
		void	topicChannel(User &user, std::vector<std::string> const &args, std::string const &topic);
		void	modeChannel(User &user, std::vector<std::string> const &args);
		void	who(User &user, std::vector<std::string> const &args) const;

	//EXCEPTIONS
	class noSuchChannel : public std::exception {
	private:
		std::string message;

	public:
		noSuchChannel(const std::string& nick, const std::string& target) {
			std::stringstream ss;
			ss << ERR_NOSUCHCHAN(nick, target);
			message = ss.str();
		}

		virtual const char* what() const throw() {
			return message.c_str();
		}
		virtual ~noSuchChannel() throw() {}
	};

	class noSuchNick : public std::exception {
	private:
		std::string message;

	public:
		noSuchNick(const std::string& nick, const std::string& target) {
			std::stringstream ss;
			ss << ERR_NOSUCHNICK(nick, target);
			message = ss.str();
		}

		virtual const char* what() const throw() {
			return message.c_str();
		}
		virtual ~noSuchNick() throw() {}
	};

	class needMoreParams : public std::exception {
	private:
		std::string message;

	public:
		needMoreParams(const std::string& nick, const std::string& command) {
			std::stringstream ss;
			ss << ERR_NEEDMOREPARAMS(nick, command);
			message = ss.str();
		}

		virtual const char* what() const throw() {
			return message.c_str();
		}
		virtual ~needMoreParams() throw() {}
	};

private:
	
	Server(void);

	std::string				_port;
	std::string				_password;
	int 					_socketServer;

	std::vector<User *>		_users;
	std::vector<Channel *>	_channels;
	int						_epollfd;
};

#endif