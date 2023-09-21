# MIPS-simulator

### Building assembly code
Run this in directories like ```<lab0?-???>/tests``` and replace ```<case>``` with the name of the test case
```bash
python3 ../../assembler.py <case>
```

### Running the simulator
Execute this line in directories like ```lab01-single-cycle```
```bash
make clean ; make && make run && make verify
```

### Tests
Run
```bash
python3 test.py <simulator_executable> <test_suite_directory>
```

For example,
```bash
python3 test.py lab01-single-cycle/MIPS.out lab01-single-cycle/tests/
```
