# Compiler and flags
CXX = g++
CXXFLAGS = -I/usr/local/include/mongocxx/v_noabi -I/usr/local/include/bsoncxx/v_noabi
LDFLAGS = -L/usr/local/lib -lmongocxx -lbsoncxx -lcurl

# Source files
SOURCES = main.cpp BitcoinCore.cpp MongoDB.cpp ProcessData.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Output binary
TARGET = documentation

# Default target
all: $(TARGET)

# Linking the final binary
$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(LDFLAGS)

# Compiling the source files
%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

# Clean target
clean:
	rm -f $(OBJECTS) $(TARGET)
