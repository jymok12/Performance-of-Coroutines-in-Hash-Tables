CXX = clang++
CXXFLAGS = -std=c++20 -stdlib=libc++ -fcoroutines-ts -g

TARGET = testHash

DATASTRUCTS = HashTable.cpp

all: $(TARGET)

$(TARGET): $(TARGET).cpp
			$(CXX) $(CXXFLAGS) -o $(TARGET) $(TARGET).cpp $(DATASTRUCTS)

.PHONY : clean
clean :
	rm -f *.o *.obj $(TARGET)