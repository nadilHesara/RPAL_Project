#ifndef LEXER_H
#define LEXER_H

typedef enum TokenType {
    TOKEN_IDENTIFIER = 0,
    TOKEN_INTEGER,
    TOKEN_OPERATOR,
    TOKEN_STRING,
    TOKEN_PUNCTUATION,
    TOKEN_KEYWORD,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct Token {
    TokenType type;
    char *lexeme;
    unsigned long line;
    unsigned long column;
} Token;

typedef struct Lexer {
    const char *source;
    unsigned long position;
    unsigned long line;
    unsigned long column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);
void token_destroy(Token *token);
const char *token_type_name(TokenType type);

#endif
