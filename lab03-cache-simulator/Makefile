CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

cachesimulator: cachesimulator.cpp
	g++ ${CXXFLAGS} cachesimulator.cpp -o cachesimulator.out
debug: cachesimulator.cpp
	g++ -DDEBUG ${CXXFLAGS} cachesimulator.cpp -o cachesimulator.out
verify:
	diff trace.txt.out expected_results/trace.txt.out.ans.txt
run:
	./cachesimulator.out cacheconfig.txt trace.txt
