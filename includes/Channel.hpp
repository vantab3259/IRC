#ifndef _CHANNEL_HPP
# define _CHANNEL_HPP

# include <vector>
# include <stdbool.h>
# include <map>

# include "Utils.hpp"
# include "User.hpp"
# include "Server.hpp"

# define CMD_JOIN(sender, chanName)										((std::string)sender + " JOIN :" + chanName + "\r\n");
# define CMD_PART(sender, chanName, reason)								((std::string)sender + " PART " + chanName + " :" + reason + "\r\n");
# define CMD_QUIT(sender, reason)										((std::string)sender + " QUIT :" + reason + "\r\n");
# define CMD_TOPIC(sender, chanName, topic)								((std::string)sender + " TOPIC " + chanName + " " + " :" + topic + "\r\n");
# define CMD_KICK(sender, chanName, kickedUser, reason)					((std::string)sender + " KICK " + chanName + " " + kickedUser + " :" + reason + "\r\n");
# define CMD_MODE(sender, chanName, sign, option, mess)					((std::string)sender + " MODE "  + chanName + " " + sign + "" + option + " " + mess + "\r\n");
# define CMD_INVITE(sender, invitedNick, chanName)						((std::string)sender + " INVITE " + invitedNick + " " + chanName + "\r\n");

# define RPL_ENDOFWHO(nickName, chanName)								((std::string)SERVER_NAME + "315 " + nickName + " " + chanName + " :End of /WHO list.\r\n");
# define RPL_CREATIONTIME(nickName, chanName, creationTime)				((std::string)SERVER_NAME + "329 " + nickName + " " + chanName + " " + creationTime + "\r\n");
# define RPL_TOPIC(nickName, chanName, chanTopic)						((std::string)SERVER_NAME + "332 " + nickName + " " + chanName + " :" + chanTopic + "\r\n");
# define RPL_TOPICWHOTIME(nickName, chanName, creator, creationTime)	((std::string)SERVER_NAME + "333 " + nickName + " " + chanName + " " + creator + " " + creationTime + "\r\n");
# define RPL_INVITE(invitingNick, invitedNick, chanName)				((std::string)SERVER_NAME + "341 " + invitingNick + " " + invitedNick  + " " + chanName +" :Invitation sent to " + invitedNick + "\r\n");
# define RPL_WHOREPLY(nickName, chanName, informationUserList)			((std::string)SERVER_NAME + "352 " + nickName + " " + chanName + " " + informationUserList + "\r\n");
# define RPL_NAMREPLY(nickName, chanName, userList)  					((std::string)SERVER_NAME + "353 " + nickName + " = " + chanName + " :" + userList + "\r\n");
# define RPL_ENDOFNAMES(nickName, chanName)								((std::string)SERVER_NAME + "366 " + nickName + " " + chanName + " :End of /NAMES list.\r\n");

# define ERR_CHANNOTINLIST(nickName, chanName, attemptedKicked)			((std::string)SERVER_NAME + "441 " + nickName + " " + chanName + " :" + attemptedKicked + " is not on that channel" + "\r\n");
# define ERR_NOTONCHANNEL(nickName, chanName)							((std::string)SERVER_NAME + "442 " + nickName + " " + chanName + " :You're not on that channel\r\n");																						   
# define ERR_CHANNELISFULL(nickname, channel)							((std::string)SERVER_NAME + "471 " + nickname + " " + channel + " :Cannot join channel, it is full" + "\r\n");
# define ERR_CHANWRONGPASS(nickname, channel)							((std::string)SERVER_NAME + "475 " + nickname + " " + channel + " :Cannot join channel (+k)" + "\r\n");
# define ERR_CHANNELUSERNOTINVIT(nickname, channel)						((std::string)SERVER_NAME + "473 " + nickname + " " + channel + " :Cannot join channel, (+i)" + "\r\n");
# define ERR_CHANOPRIVSNEEDED(nickName, chanName)						((std::string)SERVER_NAME + "482 " + chanName + " " + nickName + " :You're not channel operator" + "\r\n");

class User;
class Server;

class Channel {

public:

	//CONSTRUCTORS & DESTRUCTORS
	Channel();
	Channel(const std::string name, const std::string creator, const std::string time);
	~Channel();

	//SETTERS AND GETTERS
	void							setName(std::string const & name);
	const std::string				&getName() const;
	const std::map<User *, bool>	&getMembers() const;
	const std::string				&getTopic() const;
	const std::string				&getTopicUpdateUser() const;
	const std::string				&getTopicUpdateTimestamp() const;
	bool							isEmpty() const;
	
	//USERS MANAGEMENT
	bool	addUser(Server const &server, User &user, std::string password, bool op);
	bool	partUser(Server const &server, User &leavingUser, std::string const &reason);
	bool	kickUser(Server const &server, User &kickedUser, User &kickerUser, std::string const &reason);
	void	inviteUser(Server const &server, User &invitedUser, User &invitingUser);
	void	quit(Server const &server, User &quiter, std::string const &reason);
	void	removeUser(User & user);

	//UPDATES
	void	updateTopic(Server const &server, User &user, std::string const &topic);
	void	updateMode(Server const &server, User &user, std::vector<std::string> const &args);
	void	updateUserList();

	//INFORMATION ABOUT USERS
	bool	userIsOP(User &user);
	bool	userOnChannel(User &user);
	void	who(Server const &server, User &user, bool checkNeeded);

private:
	std::string				_name;

	std::string				_userList;
	size_t					_userLimit;
	bool					_inviteOnly;
	std::string				_creator;
	std::string				_creationTime;

	std::string		 		_topic;
	std::string		 		_topicUpdateTimestamp;
	std::string		 		_topicUpdateUser;
	bool					_topicMode;

	std::string		 		_password;
	bool					_passwordMode;

	std::string		 		_operatorList;

	std::map<User *, bool>	_members;
	std::vector<User *>		_pendingUserInvitations;
};

#endif