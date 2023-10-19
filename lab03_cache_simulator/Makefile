CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

cachesimulator: cachesimulator.cpp
	g++ ${CXXFLAGS} -g cachesimulator.cpp -o cachesimulator.out
debug: cachesimulator.cpp
	g++ -DDEBUG ${CXXFLAGS} -g cachesimulator.cpp -o cachesimulator.out
run: cachesimulator.out
	./cachesimulator.out
