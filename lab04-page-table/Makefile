CXXFLAGS = -Wall -std=c++17 -Wc++17-compat -Wc++17-compat-pedantic -Wc++17-extensions

pagetable: PageTable.cpp
	g++ ${CXXFLAGS} PageTable.cpp -o PageTable.out
debug: PageTable.cpp
	g++ -DDEBUG ${CXXFLAGS} PageTable.cpp -o PageTable.out
clean:
	rm PageTable.out
