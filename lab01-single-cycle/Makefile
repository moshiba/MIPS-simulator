CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

mips: MIPS.cpp
	g++ ${CXXFLAGS} MIPS.cpp -o MIPS.out
debug: MIPS.cpp
	g++ -DDEBUG ${CXXFLAGS} MIPS.cpp -o MIPS.out
run:
	./MIPS.out
verify:
	diff RFresult.txt expected_results/RFresult_ans.txt
	diff dmemresult.txt expected_results/dmemresult_ans.txt
clean:
	rm MIPS.out RFresult.txt dmemresult.txt
