#include "Utils.hpp"

/*Print a vector of string into stdout*/
void	displayStringVector(std::vector<std::string> strings)
{
	for (std::vector<std::string>::iterator it = strings.begin(); it != strings.end(); ++it)
        std::cout << *it << "   ";
    std::cout << std::endl;
}

/*ATOI but better*/
int	toInt(std::string s) {
	std::stringstream	ss(s);
    int					value;

    ss >> value;
	if (ss.fail() || !ss.eof()) {
		return (-1);
	}

	return (value);
}

std::string	toString(int n) {
    std::stringstream ss;
    
	ss << n;
    return (ss.str());
}

bool	isValidName(std::string const &name) {
	if (name.length() > 12)
		return (false);

	for (size_t i = 0; i < name.length(); ++i) {
        char c = name[i];
        if (!(isalnum(c) || c == '_')) {
            return (false);
        }
    }

	return (true);
}