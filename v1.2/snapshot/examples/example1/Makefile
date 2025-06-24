################################################################################
# COMPILATION
################################################################################
CPP		=	c++
FLAGS	=	-Wall -Wextra -Werror -std=c++11 $(HEADERS) -g #-fsanitize=address
HEADERS	=	-I ./incs/

################################################################################
# GENERAL
################################################################################
NAME	=	ircserv
SRC_DIR	=	./srcs

FILES	=	main.cpp \
			Channel.cpp \
			Client.cpp \
			Helpers.cpp \
			Poll.cpp \
			Server.cpp \
			ServerUtils.cpp \
			SignalHandler.cpp \
			Socket.cpp \
			capLs.cpp \
			createChannel.cpp \
			invite.cpp \
			join.cpp \
			kick.cpp \
			list.cpp \
			mode.cpp \
			nick.cpp \
			pass.cpp \
			ping.cpp \
			privmsg.cpp \
			quit.cpp \
			topic.cpp \
			user.cpp \

SOURCES = $(addprefix srcs/, $(FILES))
OBJECTS = $(addprefix objs/, $(FILES:.cpp=.o))

################################################################################
# RULES
################################################################################
vpath %.cpp $(SRC_DIR) $(SRC_DIR)/commands

all: $(NAME)

objs/%.o: %.cpp
	@$(CPP) $(FLAGS) -c $< -o $@
	@echo "$(DARK_GRAY)Compiled: $< $(X)"

$(NAME): folders $(OBJECTS)
	@$(CPP) $(FLAGS) $(OBJECTS) -o $(NAME)
	@echo "$(DARK_MAGENTA)- - ✨✨✨✨ $(NAME) compiled! ✨✨✨✨ - -$(X)"
	@echo "$(GREEN)- - - - - - - - - - - - - - - - - - - - - - -$(X)"
	@echo "$(GREEN)Run the program with ./$(NAME)$(X)"

folders:
	@mkdir -p objs/
	@echo "$(DARK_MAGENTA)- - - - - 📁 Created all folders! 📁 - - - - -$(X)"

clean:
	@rm -rf objs/ bobjs/
	@echo "$(DARK_MAGENTA)- - - - -❗All object files cleaned❗- - - - -$(X)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(DARK_MAGENTA)- - - -❗All executable files cleaned❗- - - -$(X)"

re: fclean all

.PHONY: all clean fclean re bonus

################################################################################
# EXTRA
################################################################################
X = \033[0;39m
BLACK = \033[0;30m
DARK_RED = \033[0;31m
DARK_GREEN = \033[0;32m
DARK_YELLOW = \033[0;33m
DARK_BLUE = \033[0;34m
DARK_MAGENTA = \033[0;35m
DARK_CYAN = \033[0;36m
DARK_GRAY = \033[0;90m
LIGHT_GRAY = \033[0;37m
RED = \033[0;91m
GREEN = \033[0;92m
YELLOW = \033[0;93m
BLUE = \033[0;94m
MAGENTA = \033[0;95m
CYAN = \033[0;96m
WHITE = \033[0;97m
