NAME = gomoku
TEST_NAME = run_tests

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -O3 #-lsfml-graphics -lsfml-window -lsfml-system

SFML_CFLAGS := $(shell pkg-config --cflags sfml-graphics sfml-window sfml-system)
SFML_LIBS := $(shell pkg-config --libs sfml-graphics sfml-window sfml-system)

INCLUDES = -Iinclude
OBJ_DIR = obj

SRC_GAME = $(shell find src -name "*.cpp")
SRC_TEST = $(shell find tests -name "*.cpp")

OBJ_GAME = $(SRC_GAME:%.cpp=$(OBJ_DIR)/%.o)
OBJ_TEST = $(SRC_TEST:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ_GAME)
	$(CXX) $(CXXFLAGS) $(OBJ_GAME) -o $(NAME) $(SFML_LIBS)

test: $(OBJ_TEST) $(filter-out $(OBJ_DIR)/src/main.o, $(OBJ_GAME))
	$(CXX) $(CXXFLAGS) $(OBJ_TEST) $(filter-out $(OBJ_DIR)/src/main.o, $(OBJ_GAME)) -o $(TEST_NAME)
	./$(TEST_NAME) $(SFML_LIBS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(SFML_CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_NAME)

re: fclean all

run: all
	./$(NAME)

.PHONY: all clean fclean re run test