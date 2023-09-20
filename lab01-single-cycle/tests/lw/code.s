# Aligned access with different base address and index pairs
lw r1 r0 0  # load [addr=0   + index=0]
lw r2 r0 4  # load [addr=0   + index=4]
lw r3 r0 8  # load [addr=0   + index=8]
lw r4 r1 0  # load [addr=$R1 + index=0]
lw r5 r1 4  # load [addr=$R1 + index=4]
lw r6 r1 8  # load [addr=$R1 + index=8]
# Unaligned access
lw r7 r1 1  # load [addr=$R  + index=1]
lw r8 r1 2  # load [addr=$R  + index=2]
lw r9 r1 3  # load [addr=$R  + index=3]

halt
