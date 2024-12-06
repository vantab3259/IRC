NAME	=	ircserv
CXX		=	c++ -g3

CPPFLAGS	=	-Wall
CPPFLAGS	+=	-Wextra
CPPFLAGS	+=	-Werror
CPPFLAGS	+=	-std=c++98
CPPFLAGS	+=	-I./includes

SRCDIR	=	./srcs
# HDRDIR	=	includes
OBJDIR	=	./objs

SOURCES	=	$(wildcard $(SRCDIR)/*.cpp)
HEADERS =	$(wildcard $(SRCDIR)/*.hpp)
OBJECTS	=	$(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

#****************************************************#
#*						RULES						*#
#****************************************************#

all: ${NAME}

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	$(CXX) $(CPPFLAGS) -c $< -o $@

${NAME}: ${OBJECTS}
	$(CXX) $(CPPFLAGS) ${OBJECTS} -o $@

clean:
	@rm -rf ${OBJDIR}		

fclean: clean
	@if [ -f ${NAME} ]; then rm ${NAME}; fi
	@echo "make fclean : done"

re: fclean ${NAME}

.PHONY: all clean fclean re
