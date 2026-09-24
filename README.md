# GOTO BASIC

A line-numbered BASIC interpreter written in C++17, using **Flex** for lexing and **Bison** for parsing. It's loosely modelled on **Dartmouth BASIC** (the classic dialect behind most 1970s/80s microcomputer BASICs), with a handful of personal extensions layered on top — most notably `WHILE`/`WEND`, `TRON`/`TROFF`, auto-dimensioning of arrays, and a couple of interpreter housekeeping commands (`TRON`, `CLS`, etc).

You type in a program line by line, each line starting with a line number, and `RUN` it — exactly like BASIC on a 1970s/80s minicomputer or microcomputer terminal.

```
GOTO BASIC Interpreter 1.0.0
An implementation of extended Dartmouth BASIC using C++, FLEX and BISON
Written by Steven James

>10 PRINT "HELLO, WORLD!"
OK
>RUN
HELLO, WORLD!
OK
```

## Contents

- [Building](#building)
- [Running](#running)
- [Language overview](#language-overview)
- [Data types](#data-types)
- [Operators](#operators)
- [Keyword reference](#keyword-reference)
  - [Interactive / interpreter commands](#interactive--interpreter-commands)
  - [Program statements](#program-statements)
- [Built-in functions](#built-in-functions)
- [Extensions beyond Dartmouth BASIC](#extensions-beyond-dartmouth-basic)
- [Test suite](#test-suite)
- [Example games](#example-games)
- [Project layout](#project-layout)

## Building

Requires CMake (3.24+), a C++17 compiler, and both **Flex** and **Bison** on the path.

```bash
cmake -S . -B build
cmake --build build
```

This produces the `gotobasic` executable in `build/`. `CMakeLists.txt` runs Bison over `src/basic.y` and Flex over `src/basic.l` to generate the parser/lexer, then compiles them together with the interpreter sources in `src/`.

An `ENABLE_SANITIZERS` CMake option is available for building with ASan/UBSan:

```bash
cmake -S . -B build -DENABLE_SANITIZERS=ON
```

## Running

**Interactive mode** — drops you into a `>` prompt where you can type program lines and commands:

```bash
./build/gotobasic
```

**Run a file directly** — pass a `.bas` file as the first argument and it is loaded and run immediately:

```bash
./build/gotobasic tests/19_fib_factorial.bas
```

## Language overview

A program is a series of lines, each beginning with an integer line number, e.g.:

```
10 LET X = 5
20 PRINT "X IS"; X
30 END
```

- Entering a line with a number that already exists replaces that line; entering a bare number with nothing after it deletes that line.
- Multiple statements can be placed on one line separated by `:` (e.g. `10 X=1: Y=2: PRINT X+Y`).
- Keywords are case-insensitive (`print`, `Print` and `PRINT` are all the same token).
- String variable names end in `$` (e.g. `A$`, `NAME$`); numeric variable names don't.
- `REM` (or a line starting with `REM`) marks the rest of the line as a comment.
- Lines typed without a leading line number are executed immediately rather than stored (this is how the interactive commands like `LIST`, `RUN`, `LOAD`, `SAVE` work).

## Data types

The interpreter has three underlying value kinds (see `src/value.hpp`):

| Kind | Notes |
|---|---|
| Integer | Used for whole-number literals and results |
| Real (float) | Used for decimal literals (`3.14`, `.5`) and results of math ops |
| String | Text values; string variables/functions use a trailing `$` |

Numeric and string values are distinguished by variable name: any identifier ending in `$` is a string variable/function, everything else is numeric.

## Operators

| Operator | Meaning |
|---|---|
| `+ - * / ^` | Add, subtract, multiply, divide, power |
| `= <> < <= > >=` | Equality/relational comparisons |
| `AND` `OR` `NOT` | Logical operators |
| `-` (unary) | Negation |

Precedence (low to high): `OR` → `AND` → `NOT` → relational (`= <> < <= > >=`) → `+ -` → `* /` → `^` → unary minus. Parentheses `( )` group sub-expressions as usual.

## Keyword reference

Every keyword below is recognized in `src/basic.l` (the Flex lexer) and given meaning in `src/basic.y` (the Bison grammar). Keywords are matched case-insensitively.

### Interactive / interpreter commands

These aren't stored as part of a numbered program — typing them (without a leading line number) executes them immediately.

| Keyword | Syntax | Description |
|---|---|---|
| `LIST` | `LIST` / `LIST <start>,<end>` | Lists the whole program, or just lines `<start>` to `<end>`. |
| `RUN` | `RUN` | Runs the currently loaded program from the beginning. |
| `NEW` | `NEW` | Clears the current program and all variables. |
| `DELETE` | `DELETE <line>` / `DELETE <start>,<end>` | Deletes a line or range of lines from the program. |
| `LOAD` | `LOAD "<file>"` | Loads a program from a file on disk. |
| `SAVE` | `SAVE "<file>"` | Saves the current program to a file on disk. |
| `TRON` | `TRON` | Turns on execution tracing (echoes line numbers as they run). |
| `TROFF` | `TROFF` | Turns off execution tracing. |

### Program statements

These are the statements you build numbered program lines out of.

| Keyword | Syntax | Description |
|---|---|---|
| `LET` | `LET var = expr` / `var = expr` | Assigns a value to a variable or array element. `LET` is optional. |
| `PRINT` | `PRINT [items separated by `;` or `,`]` | Prints values. A trailing `;` suppresses the newline and packs output tightly; a trailing `,` suppresses the newline and advances to the next tab (column) stop; commas between items also tab-align columns. `PRINT` with no arguments prints a blank line. |
| `INPUT` | `INPUT var[, var...]` / `INPUT "prompt", var[, var...]` | Prompts (default prompt is `? `) and reads one or more values from the user into variables. |
| `IF` | `IF expr THEN stmts [ELSE stmts]` / `IF expr THEN <line>` | Conditional. The `THEN`/`ELSE` clauses can be a `:`-separated list of statements, or a bare line number to jump to (classic `IF...THEN <linenum>` form). |
| `GOTO` | `GOTO <line>` | Unconditional jump to a line number. |
| `GOSUB` | `GOSUB <line>` | Jumps to a line number, remembering the return point. |
| `RETURN` | `RETURN` | Returns control to the statement after the most recent `GOSUB`. |
| `ON`...`GOTO` | `ON expr GOTO <line1>,<line2>,...` | Evaluates `expr` and jumps to the Nth line in the list (1-based). |
| `ON`...`GOSUB` | `ON expr GOSUB <line1>,<line2>,...` | Same idea, but as a subroutine call. |
| `FOR` | `FOR var = start TO end [STEP step]` | Begins a counted loop. `STEP` defaults to 1 if omitted. |
| `NEXT` | `NEXT` / `NEXT var` | Closes the nearest (or named) `FOR` loop, incrementing and looping back if still in range. |
| `WHILE` | `WHILE expr` | Begins a condition-checked loop *(extension — see below)*. |
| `WEND` | `WEND` | Closes the nearest `WHILE` loop, re-testing the condition. |
| `DIM` | `DIM a(size)[,b(size)...]` | Explicitly declares the bounds of one or more arrays (supports multiple dimensions, e.g. `DIM G(8,8)`). |
| `DEF` | `DEF FNname(param) = expr` | Defines a single-expression user function, e.g. `DEF FNS(X) = X*X`. Called as `FNS(4)`. |
| `DATA` | `DATA val1, val2, ...` | Declares a line of literal constants (numbers or strings) to be consumed by `READ`. |
| `READ` | `READ var[, var...]` | Reads the next value(s) from the program's `DATA` pool into variables. |
| `RESTORE` | `RESTORE` / `RESTORE <line>` | Resets the `READ` pointer back to the start of `DATA`, or to a specific `DATA` line. |
| `RANDOMIZE` | `RANDOMIZE` / `RANDOMIZE expr` | Reseeds the random number generator, optionally with a specific seed. |
| `CLEAR` | `CLEAR` | Clears all variable values (keeps the program itself). |
| `CLS` | `CLS` | Clears the screen. |
| `DIM`/auto-dim | *(see extensions)* | Arrays used without a prior `DIM` are auto-dimensioned. |
| `END` | `END` | Ends program execution. |
| `STOP` | `STOP` | Halts program execution (equivalent effect to `END`, conventionally used mid-program). |
| `EXIT` | `EXIT` | Exits the interpreter itself. |
| `REM` | `REM <comment text>` | A comment; the rest of the line is ignored. |

## Built-in functions

Recognized as function tokens in the lexer and wired up to expression nodes in the grammar.

**Math**

| Function | Description |
|---|---|
| `ABS(x)` | Absolute value |
| `ATN(x)` | Arctangent |
| `COS(x)` | Cosine |
| `EXP(x)` | e^x |
| `INT(x)` | Truncate to integer (floor) |
| `LOG(x)` | Natural logarithm |
| `RND(x)` | Random number |
| `SGN(x)` | Sign of x (-1, 0, or 1) |
| `SIN(x)` | Sine |
| `SQR(x)` | Square root |
| `TAN(x)` | Tangent |

**Strings**

| Function | Description |
|---|---|
| `LEN(s)` | Length of string `s` |
| `ASC(s)` | ASCII code of the first character of `s` |
| `CHR$(n)` | Character for ASCII code `n` |
| `STR$(x)` | Converts a number to its string representation |
| `VAL(s)` | Converts a string to a number |
| `LEFT$(s,n)` | Leftmost `n` characters of `s` |
| `RIGHT$(s,n)` | Rightmost `n` characters of `s` |
| `MID$(s,start[,len])` | Substring of `s` starting at `start`, optionally limited to `len` characters |

**Output formatting** (used inside `PRINT`)

| Function | Description |
|---|---|
| `TAB(n)` | Moves the print column to position `n` |
| `SPC(n)` | Prints `n` spaces |

## Extensions beyond Dartmouth BASIC

Dartmouth BASIC didn't have all of these; they're noted here as the author's own additions on top of the core dialect:

- **`WHILE` / `WEND`** — a condition-checked loop construct, alongside the classic `FOR`/`NEXT`.
- **`TRON` / `TROFF`** — toggle execution tracing for debugging.
- **`CLS`** — clear the screen.
- **Auto-dimensioning of arrays** — an array referenced without a prior `DIM` is automatically sized to indices `0` to `10` in each dimension (as demonstrated in `tests/24_auto_dim.bas` and `tests/45_auto_dim_multidim.bas`). Auto-dim does *not* grow — indexing past 10 on an auto-dimensioned array is a runtime error (`tests/46_auto_dim_errors.bas`), and explicitly `DIM`-ing a variable that was already auto-dimensioned by use is also a runtime error (`tests/47_redim_error.bas`).
- **`EXIT`** — quits the interpreter itself (distinct from `END`/`STOP`, which just end the running program).
- **`LOAD` / `SAVE`** — load and save programs to disk from the interactive prompt.

## Test suite

The `tests/` directory contains numbered `.bas` regression scripts, each exercising a specific piece of language functionality. Every test prints its own expected output so results can be eyeballed (or diffed) against what actually runs:

| File | Covers |
|---|---|
| `01_print_arithmetic.bas` | `PRINT` and arithmetic expressions |
| `02_let_variables.bas` | `LET` / variable assignment |
| `03_goto.bas` | `GOTO` |
| `04_if_then_else.bas` | `IF`/`THEN`/`ELSE` |
| `05_for_next.bas` | `FOR`/`NEXT` loops |
| `06_gosub_return.bas` | `GOSUB`/`RETURN` |
| `07_math_functions.bas` | Math built-ins |
| `08_trig.bas` | Trig functions |
| `09_string_functions.bas` | String built-ins |
| `10_arrays_1d.bas` | One-dimensional arrays |
| `11_arrays_2d.bas` | Two-dimensional arrays |
| `12_def_fn.bas` | `DEF FN` user functions |
| `13_on_goto_gosub.bas` | `ON...GOTO` / `ON...GOSUB` |
| `14_print_formatting.bas` | `PRINT` column/`TAB` formatting |
| `15_boolean.bas` | Boolean/logical operators |
| `16_input.bas` | `INPUT` |
| `17_stop.bas` | `STOP` |
| `18_rnd_spc.bas` | `RND` and `SPC` |
| `19_fib_factorial.bas` | Fibonacci & factorial (algorithm test) |
| `20_bubble_sort.bas` | Bubble sort (algorithm test) |
| `21_primes.bas` | Prime sieve (algorithm test) |
| `22_matrix.bas` | 2D matrix operations |
| `23_multi_statement.bas` | Multiple `:`-separated statements per line |
| `24_auto_dim.bas` | Array auto-dimensioning (1D) |
| `25_newton_raphson.bas` | Newton-Raphson method (algorithm test) |
| `26_data_read.bas` | `DATA`/`READ` |
| `27_restore.bas` | `RESTORE` |
| `28_data_types.bas` | Mixed data types in `DATA` |
| `29_data_loop.bas` | Reading `DATA` in a loop |
| `30_data_string_table.bas` | String tables via `DATA` |
| `31_randomize.bas` | `RANDOMIZE` |
| `32_clear.bas` | `CLEAR` |
| `33_while_wend.bas` | `WHILE`/`WEND` |
| `34_while_nested.bas` | Nested `WHILE`/`WEND` |
| `35_while_accumulate.bas` | `WHILE`/`WEND` accumulator pattern |
| `36_while_string_sentinel.bas` | `WHILE`/`WEND` with a string sentinel condition |
| `37_print_comma_tabs.bas` | `PRINT` comma tab-stops |
| `38_print_semicolon.bas` | `PRINT` semicolon packing |
| `39_print_tab.bas` | `PRINT TAB(n)` |
| `40_combined_features.bas` | Combined-feature smoke test |
| `41_bad_program.bas` | Deliberately invalid program (error handling) |
| `42_multi_statement_conditional.bas` | Multi-statement `IF` bodies |
| `43_mulit_statement_gosub.bas` | Multi-statement `GOSUB` bodies |
| `44_multi_statement_conditional_goto.bas` | Multi-statement conditional `GOTO` |
| `45_auto_dim_multidim.bas` | Array auto-dimensioning (2D/3D) |
| `46_auto_dim_errors.bas` | Auto-dim out-of-range error handling |
| `47_redim_error.bas` | Re-`DIM` of an auto-dimensioned array error handling |

## Example games

Alongside the numbered regression tests, `tests/` also includes a couple of classic BASIC games, useful as larger, real-world exercises of the interpreter:

- **`hangman.bas`** — the classic "Creative Computing" Hangman game (word-guessing, `DATA`-driven word list). Exercises string handling, 2D arrays, `DATA`/`READ`/`RESTORE`, and nested loops.
- **`basic_trek.bas`** — a port of *Super Star Trek* (the 1978 Mike Mayfield / Bob Leedom classic, as published in "101 BASIC Computer Games"). A large, multi-hundred-line program exercising arrays, `DEF FN`, `GOSUB`/`RETURN`, `ON...GOTO`, and heavy `PRINT` formatting — a good stress test for the interpreter.

Run either with:

```bash
./build/gotobasic tests/hangman.bas
./build/gotobasic tests/basic_trek.bas
```

## Project layout

```
src/
  basic.l              Flex lexer — tokenizes keywords, identifiers, literals, operators
  basic.y              Bison grammar — parses statements/expressions into an AST
  value.hpp/.cpp        Value type (Integer / Real / String)
  expression.hpp/.cpp   Expression AST nodes and evaluation
  expression_ops.hpp    Expression node "Make*" factory helpers
  statement.hpp/.cpp    Statement AST nodes and execution
  statement_ops.hpp     Statement node "Make*" factory helpers
  environment.hpp/.cpp  Variable/array storage and interpreter state
  program_manager.hpp/.cpp  Stores/lists/runs numbered program lines; LOAD/SAVE
  data_manager.hpp/.cpp     DATA/READ/RESTORE pool
  runtime_ip.hpp/.cpp    Runtime instruction pointer / control flow (loops, GOSUB stack)
  error.hpp/.cpp         Error types
  parse_error.hpp        Parse error line-reading helper
  main.cpp               Entry point (REPL and file-run modes)
tests/                  Numbered regression scripts (01-47) plus hangman.bas and basic_trek.bas (+ variants)
CMakeLists.txt          Build configuration (Flex/Bison + C++17)
```
