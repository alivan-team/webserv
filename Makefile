NAME		:= webserv

CXX			:= c++
CXXFLAGS	:= -Wall -Wextra -Werror -std=c++17 -g
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
			   code/client.cpp

OBJS		:= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS		:= $(OBJS:.o=.d)

RM			:= rm -rf

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

clean:
	$(RM) $(OBJ_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
