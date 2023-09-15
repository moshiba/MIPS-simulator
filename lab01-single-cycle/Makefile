mips: MIPS.cpp
	g++ -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions MIPS.cpp -o MIPS.out
run:
	./MIPS.out
verify:
	diff RFresult.txt expected_results/RFresult_ans.txt
	diff dmemresult.txt expected_results/dmemresult_ans.txt
clean:
	rm MIPS.out RFresult.txt dmemresult.txt
