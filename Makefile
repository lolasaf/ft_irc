NAME = ircserv
BOT_NAME = ircbot
SRCS_DIR = src
OBJ_DIR = obj
SRCS_FILES = main.cpp \
			 server.cpp \
			 serverUserReg.cpp \
			 serverMessage.cpp \
			 user.cpp \
			 message.cpp \
			 channel.cpp \
			 channelMembers.cpp \
			 channelModes.cpp \
			 channelTopic.cpp \
			 channelOperators.cpp \
			 serverChannel.cpp \
			 serverCommands.cpp \
			 serverCommandsMode.cpp \
			 serverUtils.cpp \
			 serverUtilsPreconditions.cpp \
			 utils.cpp

BOT_SRCS = $(SRCS_DIR)/bot.cpp \
			$(SRCS_DIR)/botHandlers.cpp \
			$(SRCS_DIR)/botParsingUtils.cpp

SRCS = $(addprefix $(SRCS_DIR)/, $(SRCS_FILES))
OBJS = $(addprefix $(OBJ_DIR)/, $(SRCS_FILES:.cpp=.o))
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I./include
all: $(NAME)

bonus: $(BOT_NAME)

$(BOT_NAME): $(BOT_SRCS)
	$(CXX) $(CXXFLAGS) -o $(BOT_NAME) $(BOT_SRCS)

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
	rm -f $(BOT_NAME)
	rm -f bot.log
re: fclean all
valgrind: $(NAME)
	valgrind --leak-check=full --show-leak-kinds=all ./$(NAME)

.PHONY: all bonus clean fclean re valgrind