CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

pagetable: PageTable.cpp
	g++ ${CXXFLAGS} PageTable.cpp -o PageTable.out
debug: PageTable.cpp
	g++ -g -DDEBUG ${CXXFLAGS} PageTable.cpp -o PageTable.out
run:
	cd sample && ./PageTable pt_requests.txt PTBR.txt
diff:
	diff sample/pt_results.txt sample/expected_results/pt_results.ans.txt
vimdiff:
	vimdiff sample/pt_results.txt sample/expected_results/pt_results.ans.txt
clean:
	rm -r PageTable.out.dSYM
