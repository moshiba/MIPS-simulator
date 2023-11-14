CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

cachesimulator: cachesimulator.cpp
	g++ ${CXXFLAGS} cachesimulator.cpp -o cachesimulator.out
debug: cachesimulator.cpp
	g++ -DDEBUG ${CXXFLAGS} cachesimulator.cpp -o cachesimulator.out
verify:
	vimdiff trace.txt.out expected_results/trace.txt.out.ans.txt
verify2:
	vimdiff trace.txt.out expected_results/trace_set_associative.txt.out.ans.txt
run:
	./cachesimulator.out cacheconfig.txt trace.txt
run2:
	./cachesimulator.out cacheconfig_set_associative.txt trace_set_associative.txt
