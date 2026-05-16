
## Building the Project

### Step 1: Navigate to the project directory

#### WSL/Linux
```bash
cd /mnt/d/Projects/RPAL_Project
```

#### Windows PowerShell
```powershell
cd D:\Projects\RPAL_Project
```

### Step 2: Clean previous builds (optional)

#### WSL/Linux
```bash
make clean
```

#### Windows PowerShell
```powershell
make clean
```

### Step 3: Build the executable

#### WSL/Linux
```bash
make
```

This creates a Linux executable named `rpal20`.

#### Windows PowerShell
```powershell
make
```

If your Windows compiler does not automatically create `rpal20.exe`, build it directly:
```powershell
gcc -Wall -Wextra -std=c99 -pedantic -o rpal20.exe rpal20.c lexer.c ast.c parser.c standardizer.c cse.c utils.c
```

This creates a Windows executable named `rpal20.exe`.

Do not mix executables between shells: a WSL-built `rpal20` runs in WSL, while a Windows-built `rpal20.exe` runs in PowerShell or Command Prompt.

## Running the Interpreter

### Basic Execution (Full Pipeline)

Run an RPAL program and output the result.

From WSL/Linux:
```bash
./rpal20 test.txt
```

From Windows PowerShell:
```powershell
.\rpal20.exe test.txt
```

If you get `Permission denied` in WSL, run:
```bash
chmod +x rpal20
```

Output: The evaluated result of the RPAL program.

### Debug Modes

#### 1. Lexer Token Output

Print all tokens from the lexer (for debugging lexical analysis):

WSL/Linux:
```bash
./rpal20 -tokens test.txt
```

Windows PowerShell:
```powershell
.\rpal20.exe -tokens test.txt
```

Output: List of tokens with line, column, type, and lexeme.

#### 2. Abstract Syntax Tree (AST)

Print the parse tree before standardization:

WSL/Linux:
```bash
./rpal20 -ast test.txt
```

Windows PowerShell:
```powershell
.\rpal20.exe -ast test.txt
```

Output: Tree structure showing the parsed program hierarchy.

#### 3. Standardized Tree (ST)

Print the standardized tree before evaluation:

WSL/Linux:
```bash
./rpal20 -st test.txt
```

Windows PowerShell:
```powershell
.\rpal20.exe -st test.txt
```

Output: Normalized tree structure ready for CSE evaluation.

## Complete Workflow Example

### WSL/Linux

```bash
# Navigate to project in WSL/Linux
cd /mnt/d/Projects/RPAL_Project

# Build
make

# Test lexer output
./rpal20 -tokens test.txt

# View parse tree
./rpal20 -ast test.txt

# View standardized tree
./rpal20 -st test.txt

# Run the program (full pipeline)
./rpal20 test.txt
```

### Windows PowerShell

```powershell
# Navigate to project in Windows
cd D:\Projects\RPAL_Project

# Build
make

# If make does not produce rpal20.exe, build directly
gcc -Wall -Wextra -std=c99 -pedantic -o rpal20.exe rpal20.c lexer.c ast.c parser.c standardizer.c cse.c utils.c

# Test lexer output
.\rpal20.exe -tokens test.txt

# View parse tree
.\rpal20.exe -ast test.txt

# View standardized tree
.\rpal20.exe -st test.txt

# Run the program (full pipeline)
.\rpal20.exe test.txt
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
