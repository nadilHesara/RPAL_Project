#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct Parser {
    Lexer *lexer;
} Parser;

void parser_init(Parser *parser, Lexer *lexer);
AstNode *parser_parse(Parser *parser);

#endif
