"""Assembler
Supports a subset of MIPS, comment lines, and inline comments.
"""
import pathlib
import re
import sys

decoder_regex = re.compile(
    # Type: Valid statement
    r"(?:^(?P<instruction>"
    r"(?P<rType>(?:addu|subu|and|or|nor))"
    r"|(?P<iType>(?:addiu|beq|lw|sw))"
    r"|(?P<jType>(?:j))"
    r"|(halt)"
    r")"  #       instruction
    r"(?:[ ]+(?P<operand1>\w+))?"  #     operand-1 (optional)
    r"(?:[ ]+(?P<operand2>\w+))?"  #     operand-2 (optional)
    r"(?:[ ]+(?P<operand3>[-\w]+))?"  #  operand-3 (optional)
    r"[ ]*(?P<inline_comment>#.*)?$)"  # in-line comment (optional)
    # Type: Comment line
    r"|(?:^(?P<comment_line>[ ]*?#.*)$)"
    # Type: Empty line
    r"|(?P<empty_line>^(?<!.)$|(?:^[ ]+$)$)",
    re.ASCII,
)

if len(sys.argv) > 1:
    assert (code_dir := pathlib.Path(sys.argv[1])).is_dir(), "not a directory"
else:
    code_dir = pathlib.Path.cwd()
assert (code := code_dir / "code.s").is_file(), "code not found"

with code.open("r") as file:
    lines = [i.strip() for i in file]

with (code_dir / "imem.txt").open("w") as file:
    for line in lines:
        tokens = decoder_regex.match(line)
        if tokens is None:
            raise RuntimeError(f"Unknown instruction: {line}")
        if tokens["instruction"]:
            if tokens["rType"]:
                opcode = 0
                rd = int(tokens["operand1"][1:])
                rs = int(tokens["operand2"][1:])
                rt = int(tokens["operand3"][1:])
                shamt = 0
                funct = {
                    "addu": 0x21,
                    "subu": 0x23,
                    "and": 0x24,
                    "or": 0x25,
                    "nor": 0x27,
                }[tokens["instruction"]]
                machine_code = f"{opcode:06b}{rs:05b}{rt:05b}{rd:05b}{shamt:05b}{funct:06b}"
            elif tokens["iType"]:
                opcode = {
                    "addiu": 0x09,
                    "beq": 0x04,
                    "lw": 0x23,
                    "sw": 0x2B,
                }[tokens["instruction"]]
                rt = int(tokens["operand1"][1:])
                rs = int(tokens["operand2"][1:])
                imm = int(tokens["operand3"]) & 0xFFFF
                machine_code = f"{opcode:06b}{rs:05b}{rt:05b}{imm:016b}"
            elif tokens["jType"]:
                opcode = 0x02
                jmp_addr = int(tokens["operand1"]) & 0x03FF_FFFF
                machine_code = f"{opcode:06b}{jmp_addr:026b}"
            else:
                machine_code = f"{(1<<32) -1:b}"

            print(machine_code)
            file.writelines([
                f"{machine_code[0:8]}\n",
                f"{machine_code[8:16]}\n",
                f"{machine_code[16:24]}\n",
                f"{machine_code[24:32]}\n",
            ])
