CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

mips: MIPS_pipeline.cpp
	g++ ${CXXFLAGS} MIPS_pipeline.cpp -o MIPS_pipeline.out
debug: MIPS_pipeline.cpp
	g++ -DDEBUG ${CXXFLAGS} MIPS_pipeline.cpp -o MIPS_pipeline.out
run:
	./MIPS_pipeline.out
verify:
	diff RFresult.txt expected_results/RFresult.txt
	diff dmemresult.txt expected_results/dmemresult.txt
	diff stateresult.txt expected_results/stateresult.txt
clean:
	rm MIPS_pipeline.out RFresult.txt dmemresult.txt stateresult.txt
