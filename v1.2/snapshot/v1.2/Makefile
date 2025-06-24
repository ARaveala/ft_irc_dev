
SOURCES = ./sources/
CPPFILES =	${SOURCES}main.cpp \
			${SOURCES}general_utilities.cpp \
			${SOURCES}Server.cpp \
			${SOURCES}serverUtilities.cpp \
			${SOURCES}epoll_utills.cpp \
			${SOURCES}Client.cpp \
			${SOURCES}signal_handler.cpp \
			${SOURCES}IrcMessage.cpp \
			${SOURCES}Channel.cpp \
			${SOURCES}CommandDispatcher.cpp \
			${SOURCES}MessageBuilder.cpp \
			${SOURCES}test.cpp
			
INCLUDES = -I ./includes
CPP = c++
CPPFLAGS = -Werror -Wextra -Wall -std=c++20 -MMD -MP $(INCLUDES) #-Wsign-conversion 
BUILDDIR = buildofiles/
OFILES = $(addprefix $(BUILDDIR), $(notdir $(CPPFILES:.cpp=.o)))
DEPFILES = $(OFILES:.o=.d)
NAME = ft_irc

# flag -Wconversion specifically prevents type conversion
all: $(BUILDDIR) $(NAME)

$(BUILDDIR)%.o: ${SOURCES}%.cpp
	$(CPP) $(CPPFLAGS) -g -c $< -o $@

$(NAME): $(OFILES)
	$(CPP) $(CPPFLAGS) $(OFILES) -o $(NAME)
	@echo "\033[0;95m Created executable\033[0m"

$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	@echo "\033[0;91m Build folder created\033[0m"

-include $(DEPFILES)

clean:
	rm -rf $(BUILDDIR)
	rm -rf $(OFILES)
	rm -rf $(DEPFILES)
	@echo "\033[0;93m build directory, ofiles and depency files cleaned out\033[0m"

fclean: clean
	rm -f $(NAME)
	@echo "\033[0;91m executbale removed\033[0m"

re: fclean all $(NAME)

.PHONY: all clean fclean re