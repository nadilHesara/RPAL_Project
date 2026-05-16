@echo off
gcc -Wall -Wextra -std=c99 -pedantic -o rpal20.exe rpal20.c lexer.c ast.c parser.c standardizer.c cse.c utils.c
