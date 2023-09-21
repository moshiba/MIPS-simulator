"""Test driver

Parameters:
    1. Simulator executable path
    2. Test folder path
"""
import itertools
import pathlib
import sys
import tempfile
import shutil
import subprocess
import re

assert len(sys.argv) > 2, "need more parameters"
assert (executable := pathlib.Path(sys.argv[1])).is_file()
assert (tests_dir := pathlib.Path(sys.argv[2])).is_dir(), "not a directory"

for child in tests_dir.iterdir():
    # Discover test cases

    imem = child / "imem.txt"
    dmem = child / "dmem.txt"
    dmem_ans = child / "dmem_ans.txt"
    rf_ans = child / "rf_ans.txt"

    if child.is_dir() and all(
            f.is_file() for f in (imem, dmem, dmem_ans, rf_ans)):
        print(f"Test: {child.name}")

        # Copy files to a temporary directory and execute the simulator
        with tempfile.TemporaryDirectory() as tmpdir_name:
            tmpdir = pathlib.Path(tmpdir_name)
            shutil.copy(executable, tmpdir / executable.name)
            shutil.copy(imem, tmpdir / imem.name)
            shutil.copy(dmem, tmpdir / dmem.name)

            subprocess.run("./MIPS.out", cwd=tmpdir, check=True)

            # Assemble results from bytes
            with (tmpdir / "RFresult.txt").open("r") as file:
                # Extract the final register file dump section
                lines = [line.strip() for line in file]
                final_section = lines[len(lines) -
                                      lines[::-1].index("A state of RF:"):]
                rf_results = [int(i, 2) for i in final_section]
            with (tmpdir / "dmemresult.txt").open("r") as file:
                # Structure the memory as words
                lines = [line.strip() for line in file]
                words = itertools.groupby(enumerate(lines), lambda x: x[0] // 4)
                dmem_results = [
                    int("".join([v for k, v in word]), 2) for idx, word in words
                ]

            # Read answers from "rf_ans.txt" and "dmem_ans.txt"
            # Assume unspecified registers are zero
            with rf_ans.open("r") as file:
                rf_ans_lines = {}
                for line in file:
                    line = line.strip()
                    if not line:  # Empty
                        continue
                    tokens = re.match((r"^R(?P<index>\d+)\s*=\s*"
                                       r"(?P<value>(0x[ABCDEF\d]+)|(\d+))"),
                                      line)
                    rf_ans_lines[int(tokens["index"])] = int(tokens["value"], 0)
                # Patch the unspecified parts with zero
                for i in range(32):
                    rf_ans_lines.setdefault(i, 0)
                rf_ans_lines = list(dict(sorted(rf_ans_lines.items())).values())
            with dmem_ans.open("r") as file:
                dmem_ans_lines = {}
                for line in file:
                    line = line.strip()
                    if not line:  # Empty
                        continue
                    tokens = re.match((r"^(?P<index>\d+)\s*=\s*"
                                       r"(?P<value>(0x[ABCDEF\d]+)|(\d+))"),
                                      line)
                    dmem_ans_lines[int(tokens["index"])] = int(
                        tokens["value"], 0)
                # Patch the unspecified parts with zero
                for i in range(1000 // 4):  # 1000 byte addressable lines
                    dmem_ans_lines.setdefault(i, 0)
                dmem_ans_lines = list(
                    dict(sorted(dmem_ans_lines.items())).values())

            # Compare answers, print diff
            # TODO: MAYBE use "difflib"?
            test_pass_flag = True
            if rf_results != rf_ans_lines:
                print("RF wrong")
                print(rf_results)
                print(rf_ans_lines)
                test_pass_flag = False
            if dmem_results != dmem_ans_lines:
                print("MEM wrong")
                print(dmem_results)
                print(dmem_ans_lines)
                test_pass_flag = False

            if test_pass_flag:
                print("- Pass")
            else:
                print("- Fail")
