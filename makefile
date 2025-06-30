CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

all: dsm_simulator generate_commands

dsm_simulator: DsmSimulator.cpp
	$(CXX) $(CXXFLAGS) -o dsm_simulator DsmSimulator.cpp

generate_commands: generate_commands.cpp
	$(CXX) $(CXXFLAGS) -o generate_commands generate_commands.cpp

clean:
	rm -f dsm_simulator generate_commands *.o
