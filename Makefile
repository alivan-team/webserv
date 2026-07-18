NAME		:= webserv
TEST_NAME	:= webserv_tests

CXX			:= g++-16
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++17
CPPFLAGS	:= -Icode/hpp

OBJ_DIR		:= build

# Add new production source files here.
# Keep test files out of SRCS when they define their own main().
SRCS		:= main.cpp \
			   code/ConfigParser.cpp \
			   code/ServerManager.cpp \
			   code/ServerConfig.cpp \
			   code/LocationConfig.cpp \
			   code/ErrorPages.cpp \
			   code/client.cpp \
			   code/printDebug.cpp \
			   code/ClientData.cpp \
			   code/HTTPRequest.cpp \
			   code/HTTPRequestParser.cpp \
			   code/HTTPResponseBuild.cpp \
			   code/HTTPResponse.cpp \

TEST_SRCS	:= tests/TestMain.cpp \
			   code/ConfigParser.cpp \
			   code/ServerConfig.cpp \
			   code/LocationConfig.cpp \
			   code/client.cpp \
			   code/ClientData.cpp \
			   code/HTTPRequest.cpp \
			   code/HTTPRequestParser.cpp \
			   code/HTTPResponse.cpp

OBJS		:= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS		:= $(OBJS:.o=.d)
TEST_OBJS	:= $(TEST_SRCS:%.cpp=$(OBJ_DIR)/%.o)
TEST_DEPS	:= $(TEST_OBJS:.o=.d)

RM			:= rm -rf

all: $(NAME)

test: $(TEST_NAME)
	./$(TEST_NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(TEST_NAME): $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) $(TEST_OBJS) -o $(TEST_NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME) $(TEST_NAME)

re: fclean all

-include $(DEPS) $(TEST_DEPS)

.PHONY: all clean fclean re test
