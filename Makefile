NAME = ircserv
SRCS_DIR = src
OBJ_DIR = obj
SRCS_FILES = main.cpp \
			 server.cpp \
			 serverUserReg.cpp \
			 serverMessage.cpp \
			 user.cpp \
			 message.cpp \
			 channel.cpp \
			 serverChannel.cpp \
			 serverCommands.cpp \
			 serverCommandsMode.cpp \
			 serverUtils.cpp \
			 serverUtilsPreconditions.cpp \
			 utils.cpp

SRCS = $(addprefix $(SRCS_DIR)/, $(SRCS_FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS_FILES:.cpp=.o))
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include
all: $(NAME)
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
$(OBJ_DIR)/%.o: $(SRCS_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(SRCS_DIR)/*.o
fclean: clean
	rm -f $(NAME)
re: fclean all
valgrind: $(NAME)
	valgrind --leak-check=full --show-leak-kinds=all ./$(NAME)

.PHONY: all clean fclean re valgrind