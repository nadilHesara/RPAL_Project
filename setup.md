
## Building the Project

### Step 1: Navigate to the project directory
```powershell
cd RPAL_Project
```

### Step 2: Clean previous builds (optional)
```powershell
make clean
```

### Step 3: Build the executable
```powershell
make
```

This will compile the lexer, parser, standardizer, and CSE evaluator into the `rpal20` executable.

## Running the Interpreter

### Basic Execution (Full Pipeline)

Run an RPAL program and output the result:
```powershell
.\rpal20 test.txt
```

Output: The evaluated result of the RPAL program.

### Debug Modes

#### 1. Lexer Token Output

Print all tokens from the lexer (for debugging lexical analysis):
```powershell
.\rpal20 -tokens test.txt
```

Output: List of tokens with line, column, type, and lexeme.

#### 2. Abstract Syntax Tree (AST)

Print the parse tree before standardization:
```powershell
.\rpal20 -ast test.txt
```

Output: Tree structure showing the parsed program hierarchy.

#### 3. Standardized Tree (ST)

Print the standardized tree before evaluation:
```powershell
.\rpal20 -st test.txt
```

Output: Normalized tree structure ready for CSE evaluation.

## Complete Workflow Example

```powershell
# Navigate to project
cd C:\Users\User\Desktop\RPAL_Project

# Build
make

# Test lexer output
.\rpal20 -tokens test.txt

# View parse tree
.\rpal20 -ast test.txt

# View standardized tree
.\rpal20 -st test.txt

# Run the program (full pipeline)
.\rpal20 test.txt
```

## Project Architecture

- **Lexer** (`lexer.c`, `lexer.h`): Tokenizes RPAL source code
- **Parser** (`parser.c`, `parser.h`): Builds Abstract Syntax Tree (AST)
- **Standardizer** (`standardizer.c`, `standardizer.h`): Normalizes AST into standardized form
- **CSE Machine** (`cse.c`, `cse.h`): Evaluates standardized tree
- **AST** (`ast.c`, `ast.h`): Abstract syntax tree node structures
- **Main** (`rpal20.c`): CLI entry point and pipeline orchestration

## Sample Input File (test.txt)

```
let rec SumN N =
	N eq 0 -> 0
	| N + SumN (N-1)
in Print (SumN 9)
```

Expected output: `45`
