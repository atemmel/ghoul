CXXFLAGS=-std=c++17 
all: 
	g++ *.cpp -o lang $(CXXFLAGS) -DDEBUG `llvm-config --ldflags --system-libs --libs core` -std=c++17 -fexceptions -g

