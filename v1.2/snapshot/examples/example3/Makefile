NAME = ircserv
BOT_NAME = bot_v1
SRC_DIR = src
OBJ_DIR = obj/
BOT_OBJ_DIR = obj/bot/
BOT_DIR = bot_src
CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++17 #-g -fsanitize=address

RM = rm -rf
HEADERS := $(shell find $(SRC_DIR) -type f -name "*.h")
SRCS := $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)%.o)

BOT_HEADERS := $(shell find $(BOT_DIR) -type f -name "*.h")
BOT_SRCS := $(shell find $(BOT_DIR) -type f -name "*.cpp")
BOT_OBJS = $(BOT_SRCS:$(BOT_DIR)/%.cpp=$(BOT_OBJ_DIR)%.o)

ifdef DEBUG
CFLAGS += -g -DDEBUG_MODE
endif

all: $(NAME)
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

bot: $(BOT_NAME) 
$(BOT_NAME): $(BOT_OBJS)
	$(CC) $(CFLAGS) $^ -o $@


$(BOT_OBJ_DIR)%.o: $(BOT_DIR)/%.cpp $(BOT_HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ_DIR) $(BOT_OBJ_DIR)

fclean: clean
	$(RM) $(NAME) $(BOT_NAME)

re: fclean all

.PHONY: all clean fclean re bot
