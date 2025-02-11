# Variables
CXX = g++
CXXFLAGS = -Wall -g
SDL2_FLAGS = `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_ttf

# Target
TARGET = chess
SRCS_DIR = src
SRCS = $(wildcard $(SRCS_DIR)/*.cpp)

# Build rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(SDL2_FLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean