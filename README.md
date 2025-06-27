# VeriGen 
### A Verilog Fuzzer with focusing on the Generate and Hierarchical Naming constructs

## User Guide
### Building
To use the fuzzer, build it with the following command in the root directory:

`g++ -std=c++17 -O2 -Wall -Iextern/indicators/include -o fuzz_run main.cpp`

Dependencies:
- A C++17 toolchain (e.g. GCC or Clang)
- indicators library in extern/indicators
- EDA tools of choice installed and on $PATH (Quartus, Vivado, ModelSim, Icarus, etc.)
  - These can also be set manually within each tool interface in `\tools`
 
### Basic Invocation

`./fuzz_run [options]`

By default, it:
- Generates a single “loop-only” design (--hier off).
- Checks that the simulated result matches the fuzzer’s internal oracle.
- Reports any crashes, mismatches and time-outs.

### Command Line Options
| Option                   | Meaning                                                                               | Default      |
| ------------------------ | ------------------------------------------------------------------------------------- | ------------ |
| `-n, --iter <N>`         | Number of designs to generate & test                                                  | `1`          |
| `-s, --seed <S>`         | RNG seed (for reproducibility)                                                        | random       |
| `-t, --tool <1–6>`       | Toolchain: 1=Quartus, 2=QuartusPro, 3=Vivado, 4=Icarus, 5=ModelSim, 6=CompareSim      |              |
| `-c, --chat`             | Verbose (“chatty”) output                                                             | off          |
| `--hier`                 | Use the **hierarchical** generator (else: loop-generator)                             | off          |
| **Loop-generator knobs** |                                                                                       |              |
| `--min-start <K>`        | Minimum loop start index                                                              | `0`          |
| `--max-start <K>`        | Maximum loop start index                                                              | `0`          |
| `--min-iter <M>`         | Minimum number of loop iterations                                                     | `2`          |
| `--max-iter <M>`         | Maximum number of loop iterations                                                     | `16`         |
| `--no-rand-update`       | Disable random choice of increment/decrement in loops                                 | on           |
| **Hier-generator knobs** |                                                                                       |              |
| `--depth <D>`            | Max hierarchy depth                                                                   | `2`          |
| `--min-child <C>`        | Minimum children per node                                                             | `2`          |
| `--max-child <C>`        | Maximum children per node                                                             | `4`          |
| `--root-prefix`          | Emit absolute (`$root...`) hierarchical names                                         | off          |
| `--defparam`             | Enable `defparam` overrides in generated modules                                      | off          |
| `--include-gen`          | Allow generate-blocks inside hierarchy leaves                                         | off          |
| `--gen-prob <p>`         | When `--include-gen`, probability (0–1) that a leaf is a `generate`-module            | `0.5`        |
| **Emit-only mode**       |                                                                                       |              |
| `--emit-file <file.v>`   | Just generate Verilog(s), write to `<file>.v` (or `<file>_NN.v` if `-n>1`), then exit | —            |

### Examples
#### Generate 10 loop-only tests under Icarus

`./fuzz_run -n 10 -t 4`

#### Generate 20 hierarchical tests (depth 5, 3–6 children) under Vivado

`./fuzz_run --hier --depth 5 --min-child 3 --max-child 6 -n 20 -t 3`

#### Dump a single Verilog file without running tools

`./fuzz_run --emit-file mytest.v --hier --depth 4`

_writes mytest.v using HierarchyGen, then exits_

#### Dump 5 numbered files

`./fuzz_run -n 5 --emit-file example.v`

_writes example_00.v, example_01.v, …, example_04.v_

### Output and Reporting
Progress bar shows current iteration, crash/mismatch/timeout counts.

On completion, you get a summary:
```
 __      __       _  _____            
 \ \    / /      (_)/ ____|           
  \ \  / /__ _ __ _| |  __  ___ _ __  
   \ \/ / _ \ '__| | | |_ |/ _ \ '_ \ 
    \  /  __/ |  | | |__| |  __/ | | |
     \/ \___|_|  |_|\_____|\___|_| |_|

=============== Summary ===============
      Iterations : 100
      Crashes    :  2
      Mismatches :  1
      Time-outs  :  0
      Seed       : 123456789
Artefacts in build/…
```
Exit codes:
- 0 – no crashes/mismatches/timeouts
- 1 – ≥1 mismatch
- 2 – ≥1 timeout
- 3 – ≥1 crash

### Troubleshooting
- Tool not on $PATH: ensure quartus_sh, vivado, vlogan, iverilog, or vsim are accessible.
- Permission errors: verify you can create/write under build/.
- Slow synthesis: using a depth of over 5 will significantly slow down the fuzzer.

### Extending VeriGen
- Add a new Verilog construct: subclass Stmt or Expr in ast.hpp, implement emit() (and eval(), for Expr).
- Hook into a new tool: derive a Tool in tools/, implement run(…) to invoke your flow and return a ToolResult.
For more details see AST framework (ast.hpp) and tool wrappers (tools/*.hpp).
