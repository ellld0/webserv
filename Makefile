NAME = webserv
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
SRC_DIRS = src \
           src/config \
		   src/network \
		   src/http \
		   src/utils
OBJ_DIR = obj/
SRC = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.cpp))
OBJ = $(patsubst src/%.cpp, $(OBJ_DIR)%.o, $(SRC))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re