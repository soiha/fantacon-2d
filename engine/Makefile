# Compiler and flags
CXX = clang++
SDL2_CFLAGS := $(shell sdl2-config --cflags)
SDL2_LIBS := $(shell sdl2-config --libs)
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude $(SDL2_CFLAGS)
LDFLAGS = $(SDL2_LIBS) -lSDL2_image -lSDL2_ttf

# Directories
SRC_DIR = src
EXAMPLE_DIR = examples
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
ENGINE_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
ENGINE_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(ENGINE_SOURCES))

EXAMPLE_SOURCES = $(wildcard $(EXAMPLE_DIR)/*.cpp)
EXAMPLE_OBJECTS = $(patsubst $(EXAMPLE_DIR)/%.cpp,$(OBJ_DIR)/example_%.o,$(EXAMPLE_SOURCES))

# Targets
LIBRARY = $(BUILD_DIR)/libengine.a
EXAMPLE = $(BUILD_DIR)/example

.PHONY: all clean example

all: $(LIBRARY) $(EXAMPLE)

# Build static library
$(LIBRARY): $(ENGINE_OBJECTS) | $(BUILD_DIR)
	ar rcs $@ $^

# Build example
$(EXAMPLE): $(EXAMPLE_OBJECTS) $(LIBRARY) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(EXAMPLE_OBJECTS) $(LIBRARY) $(LDFLAGS)

# Compile engine source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile example source files
$(OBJ_DIR)/example_%.o: $(EXAMPLE_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

example: $(EXAMPLE)
	@echo "Example built at: $(EXAMPLE)"
	@echo "Run with: ./$(EXAMPLE)"
