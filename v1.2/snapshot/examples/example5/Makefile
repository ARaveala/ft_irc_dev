NAME		=	ircserv

SRC			=	src/main.cpp \
				src/Channel.cpp \
				src/channelUtils.cpp \
				src/Client.cpp \
				src/clientLoop.cpp \
				src/connectionMessages.cpp \
				src/handleUserInput.cpp \
				src/initializeMsg.cpp \
				src/invite.cpp \
				src/join.cpp \
				src/joinChecks.cpp \
				src/kick.cpp \
				src/makeMessages.cpp \
				src/makeSelectAndRunCommand.cpp \
				src/mode.cpp \
				src/part.cpp \
				src/privmsg.cpp \
				src/Server.cpp \
				src/serverInit.cpp \
				src/serverLoop.cpp \
				src/timeUtils.cpp \
				src/topic.cpp \
				src/who.cpp \

OBJ			=	$(SRC:.cpp=.o)

CC			=	c++

# CFLAGS		=	-Wextra -Wall -Werror

# for mac, it requires the -std=c++11 flag 
CFLAGS		=	-Wextra -Wall -Werror -std=c++11

RM			=	rm -f




all: 		$(NAME)

$(NAME): $(OBJ)
			$(CC) $(CFLAGS) -g $(OBJ) -o $(NAME) 

%.o:%.cpp
			$(CC) $(CFLAGS) -c $< -o $@

clean:
			${RM} $(OBJ)

fclean: 	clean
			${RM} $(NAME)

re:			fclean all

.PHONY:		all clean fclean re