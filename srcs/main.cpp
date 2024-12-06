#include "Server.hpp"
#include "Utils.hpp"

bool	g_end;

void handleSignal(int signum) 
{
	if (signum == SIGINT)
		g_end = true;
}

int main(int argc, char **argv)
{	
	try {
		if (argc != 3)
			throw std::runtime_error("usage ./ircserv <port> <password>");
		signal(SIGINT, handleSignal);
		Server server(argv[1], argv[2]);
		server.run();

	} catch (std::exception const &e) {
		std::cerr << e.what() << std::endl;
	}

	return (0);
}
