NAME = gomo
TEST_NAME = run_tests

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -O3 -DSFML_STATIC

LOCAL_PREFIX = $(HOME)/local

SFML_CFLAGS := -I$(LOCAL_PREFIX)/include $(shell pkg-config --cflags freetype2 2>/dev/null)
SFML_LIBS   := -L$(LOCAL_PREFIX)/lib \
               -lsfml-graphics-s -lsfml-window-s -lsfml-system-s \
               -lGL -lfreetype -lX11 -lXrandr -lXcursor -lXi -ludev

INCLUDES = -Iinclude
OBJ_DIR = obj

SRC_GAME = $(shell find src -name "*.cpp")
SRC_TEST = $(shell find tests -name "*.cpp")

OBJ_GAME = $(SRC_GAME:%.cpp=$(OBJ_DIR)/%.o)
OBJ_TEST = $(SRC_TEST:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ_GAME)
	$(CXX) $(CXXFLAGS) $(OBJ_GAME) -o $(NAME) $(SFML_LIBS)

test: $(OBJ_TEST) $(filter-out $(OBJ_DIR)/src/app/main.o, $(OBJ_GAME))
	$(CXX) $(CXXFLAGS) $(OBJ_TEST) $(filter-out $(OBJ_DIR)/src/app/main.o, $(OBJ_GAME)) -o $(TEST_NAME) $(SFML_LIBS)
	./$(TEST_NAME)

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