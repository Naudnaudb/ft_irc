# Compiler flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

# Source files
SRCS = main.cpp IrcServer.cpp

# Header files
HDRS = *.hpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Executable file
NAME = ircserv

# Default target
all: $(NAME)

# Link object files to create executable
$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $@

# Compile source files to object files
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re