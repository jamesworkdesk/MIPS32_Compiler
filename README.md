# MIPS32 Assembler

A MIPS32 assembler that converts assembly source into MIF (Memory Initialization File) format, suitable for loading into RAM/ROM on FPGAs.

Originally written by James Mak (2016), modernized in 2026.

## Build

Requires a C++17 compiler (g++ recommended).

```
make
```

## Usage

```
./assembler Input.txt
```

This reads `Input.txt` and produces `Input.mif` containing the assembled machine code.

## Supported Instructions (29)

| Type | Instructions |
|------|-------------|
| R-type (arithmetic) | `add`, `addu`, `sub`, `subu`, `and`, `or`, `nor`, `slt`, `sltu` |
| R-type (shift) | `sll`, `srl` |
| R-type (jump) | `jr` |
| I-type (arithmetic) | `addi`, `addiu`, `andi`, `ori`, `slti`, `sltiu` |
| I-type (load upper) | `lui` |
| I-type (branch) | `beq`, `bne` |
| I-type (memory) | `lw`, `sw`, `lbu`, `lhu`, `sb`, `sh` |
| J-type | `j`, `jal` |

## Pseudo-Instructions

| Pseudo | Expansion |
|--------|-----------|
| `nop` | `sll $0, $0, 0` |
| `move $d, $s` | `add $d, $s, $0` |
| `li $d, imm` | `ori $d, $0, imm` (if imm fits 16 bits) or `lui $d, upper` + `ori $d, $d, lower` |

## Register Aliases

Both numeric (`$0`-`$31`) and named registers are supported:

| Alias | Register | Alias | Register | Alias | Register | Alias | Register |
|-------|----------|-------|----------|-------|----------|-------|----------|
| `$zero` | `$0` | `$t0` | `$8` | `$s0` | `$16` | `$t8` | `$24` |
| `$at` | `$1` | `$t1` | `$9` | `$s1` | `$17` | `$t9` | `$25` |
| `$v0` | `$2` | `$t2` | `$10` | `$s2` | `$18` | `$k0` | `$26` |
| `$v1` | `$3` | `$t3` | `$11` | `$s3` | `$19` | `$k1` | `$27` |
| `$a0` | `$4` | `$t4` | `$12` | `$s4` | `$20` | `$gp` | `$28` |
| `$a1` | `$5` | `$t5` | `$13` | `$s5` | `$21` | `$sp` | `$29` |
| `$a2` | `$6` | `$t6` | `$14` | `$s6` | `$22` | `$fp` | `$30` |
| `$a3` | `$7` | `$t7` | `$15` | `$s7` | `$23` | `$ra` | `$31` |

## Features

- Comment support: lines or trailing comments with `#`
- Case-insensitive hex values: `0xFFFF` and `0xffff` both work
- Error messages with line numbers
- Output filename derived from input (`Input.txt` -> `Input.mif`)
- Decoded instruction comments in MIF output

## Testing

```
make test
```

Assembles `Input.txt` and compares the output against the reference `Output.mif`.
