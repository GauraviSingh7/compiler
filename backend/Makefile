# Makefile — builds the Pascal compiler binary

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

SRC_DIR = ../compiler-src

# Automatically pick all .cpp files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

TARGET = compiler

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "✓ Built $(TARGET)"

clean:
	rm -f $(TARGET) *.o