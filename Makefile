CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -pthread
SRC = src/PortScanner.cpp src/main.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = port_scanner

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
