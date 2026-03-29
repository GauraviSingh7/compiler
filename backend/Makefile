# Makefile — builds the Pascal compiler binary (Linux-ready)

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# Source directory
SRC_DIR = ../compiler-src

# All source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Output binary (NO .exe for Linux)
TARGET = compiler

.PHONY: all clean rebuild

# Default build
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Built $(TARGET) successfully"

# Clean build files
clean:
	rm -f $(TARGET) *.o

# Force rebuild
rebuild: clean all