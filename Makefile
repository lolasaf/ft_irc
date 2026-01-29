NAME = ircserv
SRCS_DIR = src
OBJ_DIR = obj
SRCS_FILES = main.cpp server.cpp serverUserReg.cpp serverMessage.cpp \
             user.cpp message.cpp channel.cpp serverChannel.cpp \
             serverCommands.cpp serverCommandsMode.cpp serverUtils.cpp \
             serverUtilsPreconditions.cpp utils.cpp

SRCS = $(addprefix $(SRCS_DIR)/, $(SRCS_FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS_FILES:.cpp=.o))

BOT_NAME = ircbot
BOT_SRCS_FILES = botMain.cpp bot.cpp
BOT_OBJS = $(addprefix $(OBJ_DIR)/, $(BOT_SRCS_FILES:.cpp=.o))

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

$(OBJ_DIR)/%.o: $(SRCS_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

bot: $(BOT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(BOT_NAME) $(BOT_OBJS)

bonus: all bot

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(SRCS_DIR)/*.o

fclean: clean
	rm -f $(NAME)
	rm -f $(BOT_NAME)

re: fclean all

valgrind: $(NAME)
	valgrind --leak-check=full --show-leak-kinds=all ./$(NAME)

.PHONY: all clean fclean re valgrind bot bonus
