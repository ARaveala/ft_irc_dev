NAME		=	ircserv
NAME_ASAN	=	ircsan
COMP		=	c++
FLAGS		=	-Wall -Werror -Wextra -std=c++17 -g
ASAN_FLAGS	= 	-Wall -Werror -Wextra -std=c++17 -fsanitize=address

SRC_DIR		=	src
OBJ_DIR		=	obj
INC_DIR		=	inc

SRC			=	$(SRC_DIR)/main.cpp\
				$(SRC_DIR)/Server.cpp\
				$(SRC_DIR)/Client.cpp\
			    $(SRC_DIR)/Message.cpp\
				$(SRC_DIR)/Channel.cpp


OBJ 		= $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

asan: $(NAME_ASAN)

$(NAME_ASAN): $(OBJ)
	@$(COMP) $(ASAN_FLAGS) -o $@ $^
	@echo "Compiling: $(NAME_ASAN)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(COMP) $(ASAN_FLAGS) -I$(INC_DIR) -c $< -o $@

$(NAME): $(OBJ)
	@$(COMP) $(FLAGS) -o $@ $^
	@echo "Compiling: $(NAME)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@$(COMP) $(FLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@echo "removing objects"

fclean: clean
	@rm -rf $(NAME)
	@echo "removing executable: $(NAME)"

re: fclean all

.PHONY: re all clean fclean