CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

branchsimulator: branchsimulator.cpp
	g++ ${CXXFLAGS} branchsimulator.cpp -o branchsimulator.out
debug: branchsimulator.cpp
	g++ -g -DDEBUG ${CXXFLAGS} branchsimulator.cpp -o branchsimulator.out
run:
	./branchsimulator.out config.txt trace.txt
diff:
	diff trace.txt.out expected_results/trace.txt.out.ans.txt
vimdiff:
	vimdiff trace.txt.out expected_results/trace.txt.out.ans.txt
linediff:
	vimdiff <( nl trace.txt.out ) <( nl expected_results/trace.txt.out.ans.txt )
clean:
	rm -r branchsimulator.out.dSYM branchsimulator.out
test:
	make debug && make run && make diff || make vimdiff
