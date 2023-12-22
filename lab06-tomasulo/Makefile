CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

tomasulosimulator: tomasulosimulator.cpp
	g++ ${CXXFLAGS} tomasulosimulator.cpp -o tomasulosimulator.out
debug: tomasulosimulator.cpp
	g++ -g -DDEBUG ${CXXFLAGS} tomasulosimulator.cpp -o tomasulosimulator.out
run:
	./tomasulosimulator.out config.txt trace.txt
diff:
	diff trace.out.txt expected_results/trace.out.txt.ans.txt
vimdiff:
	vimdiff trace.out.txt expected_results/trace.out.txt.ans.txt
linediff:
	vimdiff <( nl trace.out.txt ) <( nl expected_results/trace.out.txt.ans.txt )
clean:
	rm -r branchsimulator.out.dSYM branchsimulator.out trace.out.txt
test:
	make debug && make run && make diff || make vimdiff

	g++ -g tomasulosimulator.cpp -o tomasulosimulator.out
