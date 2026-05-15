#include "lexer.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static int is_letter(char value)
{
    return isalpha((unsigned char)value);
}

static int is_digit(char value)
{
    return isdigit((unsigned char)value);
}

static int is_identifier_part(char value)
{
    return is_letter(value) || is_digit(value) || value == '_';
}

static int is_operator_symbol(char value)
{
    switch (value) {
    case '+':
    case '-':
    case '*':
    case '<':
    case '>':
    case '&':
    case '.':
    case '@':
    case '/':
    case ':':
    case '=':
    case '~':
    case '|':
    case '$':
    case '!':
    case '#':
    case '%':
    case '^':
    case '_':
    case '[':
    case ']':
    case '{':
    case '}':
    case '"':
    case '`':
    case '?':
        return 1;
    default:
        return 0;
    }
}

static int is_punctuation(char value)
{
    return value == '(' || value == ')' || value == ';' || value == ',';
}

static int is_keyword(const char *text)
{
    static const char *keywords[] = {
        "let", "in", "where", "fn", "within", "rec",
        "true", "false", "nil", "dummy"
    };
    size_t index = 0;

    for (index = 0; index < sizeof(keywords) / sizeof(keywords[0]); index++) {
        if (strcmp(text, keywords[index]) == 0) {
            return 1;
        }
    }

    return 0;
}

static int is_word_operator(const char *text)
{
    static const char *operators[] = {
        "aug", "or", "not", "gr", "ge", "ls", "le", "eq", "ne", "and"
    };
    size_t index = 0;

    for (index = 0; index < sizeof(operators) / sizeof(operators[0]); index++) {
        if (strcmp(text, operators[index]) == 0) {
            return 1;
        }
    }

    return 0;
}

static char current_char(const Lexer *lexer)
{
    return lexer->source[lexer->position];
}

static char peek_char(const Lexer *lexer)
{
    if (current_char(lexer) == '\0') {
        return '\0';
    }

    return lexer->source[lexer->position + 1];
}

static void advance(Lexer *lexer)
{
    char value = current_char(lexer);

    if (value == '\0') {
        return;
    }

    lexer->position++;

    if (value == '\r') {
        if (current_char(lexer) == '\n') {
            lexer->position++;
        }
        lexer->line++;
        lexer->column = 1;
    } else if (value == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
}

static char *copy_lexeme(const char *source, unsigned long start, unsigned long end)
{
    size_t length = (size_t)(end - start);
    char *lexeme = (char *)malloc(length + 1);

    if (lexeme == NULL) {
        return NULL;
    }

    memcpy(lexeme, source + start, length);
    lexeme[length] = '\0';
    return lexeme;
}

static Token make_token(TokenType type, char *lexeme,
                        unsigned long line, unsigned long column)
{
    Token token;

    token.type = type;
    token.lexeme = lexeme;
    token.line = line;
    token.column = column;
    return token;
}

static Token make_simple_token(TokenType type, const char *source,
                               unsigned long start, unsigned long end,
                               unsigned long line, unsigned long column)
{
    return make_token(type, copy_lexeme(source, start, end), line, column);
}

static Token make_error_token(const char *message,
                              unsigned long line, unsigned long column)
{
    char *lexeme = copy_lexeme(message, 0, (unsigned long)strlen(message));

    return make_token(TOKEN_ERROR, lexeme, line, column);
}

static int is_space(char value)
{
    return value == ' ' || value == '\t' || value == '\n' || value == '\r';
}

static void skip_spaces_and_comments(Lexer *lexer)
{
    int skipped = 1;

    while (skipped) {
        skipped = 0;

        while (is_space(current_char(lexer))) {
            advance(lexer);
            skipped = 1;
        }

        if (current_char(lexer) == '/' && peek_char(lexer) == '/') {
            while (current_char(lexer) != '\0' &&
                   current_char(lexer) != '\n' &&
                   current_char(lexer) != '\r') {
                advance(lexer);
            }
            skipped = 1;
        }
    }
}

static Token lex_identifier_or_reserved(Lexer *lexer)
{
    unsigned long start = lexer->position;
    unsigned long line = lexer->line;
    unsigned long column = lexer->column;
    char *lexeme = NULL;

    while (is_identifier_part(current_char(lexer))) {
        advance(lexer);
    }

    lexeme = copy_lexeme(lexer->source, start, lexer->position);
    if (lexeme == NULL) {
        return make_error_token("out of memory", line, column);
    }

    if (is_keyword(lexeme)) {
        return make_token(TOKEN_KEYWORD, lexeme, line, column);
    }

    if (is_word_operator(lexeme)) {
        return make_token(TOKEN_OPERATOR, lexeme, line, column);
    }

    return make_token(TOKEN_IDENTIFIER, lexeme, line, column);
}

static Token lex_integer(Lexer *lexer)
{
    unsigned long start = lexer->position;
    unsigned long line = lexer->line;
    unsigned long column = lexer->column;

    while (is_digit(current_char(lexer))) {
        advance(lexer);
    }

    return make_simple_token(TOKEN_INTEGER, lexer->source, start, lexer->position,
                             line, column);
}

static int is_string_char(char value)
{
    return value == ' ' || is_letter(value) || is_digit(value) ||
           is_operator_symbol(value) || is_punctuation(value);
}

static Token lex_string(Lexer *lexer)
{
    unsigned long start = lexer->position;
    unsigned long line = lexer->line;
    unsigned long column = lexer->column;

    advance(lexer);

    while (current_char(lexer) != '\0' && current_char(lexer) != '\'') {
        if (current_char(lexer) == '\\') {
            char escaped = peek_char(lexer);

            if (escaped == 't' || escaped == 'n' || escaped == '\\' ||
                escaped == '\'') {
                advance(lexer);
                advance(lexer);
                continue;
            }

            return make_error_token("invalid string escape", lexer->line,
                                    lexer->column);
        }

        if (!is_string_char(current_char(lexer))) {
            return make_error_token("invalid string character", lexer->line,
                                    lexer->column);
        }

        advance(lexer);
    }

    if (current_char(lexer) != '\'') {
        return make_error_token("unterminated string", line, column);
    }

    advance(lexer);
    return make_simple_token(TOKEN_STRING, lexer->source, start, lexer->position,
                             line, column);
}

static Token lex_operator(Lexer *lexer)
{
    unsigned long start = lexer->position;
    unsigned long line = lexer->line;
    unsigned long column = lexer->column;

    while (is_operator_symbol(current_char(lexer))) {
        advance(lexer);
    }

    return make_simple_token(TOKEN_OPERATOR, lexer->source, start, lexer->position,
                             line, column);
}

void lexer_init(Lexer *lexer, const char *source)
{
    if (lexer == 0) {
        return;
    }

    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
}

Token lexer_next_token(Lexer *lexer)
{
    unsigned long start = 0;
    unsigned long line = 0;
    unsigned long column = 0;

    skip_spaces_and_comments(lexer);

    start = lexer->position;
    line = lexer->line;
    column = lexer->column;

    if (current_char(lexer) == '\0') {
        return make_token(TOKEN_EOF, NULL, line, column);
    }

    if (is_letter(current_char(lexer))) {
        return lex_identifier_or_reserved(lexer);
    }

    if (is_digit(current_char(lexer))) {
        return lex_integer(lexer);
    }

    if (current_char(lexer) == '\'') {
        return lex_string(lexer);
    }

    if (is_punctuation(current_char(lexer))) {
        advance(lexer);
        return make_simple_token(TOKEN_PUNCTUATION, lexer->source, start,
                                 lexer->position, line, column);
    }

    if (is_operator_symbol(current_char(lexer))) {
        return lex_operator(lexer);
    }

    advance(lexer);
    return make_error_token("unrecognized character", line, column);
}

void token_destroy(Token *token)
{
    if (token == NULL) {
        return;
    }

    free(token->lexeme);
    token->lexeme = NULL;
}

const char *token_type_name(TokenType type)
{
    switch (type) {
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_INTEGER:
        return "INTEGER";
    case TOKEN_OPERATOR:
        return "OPERATOR";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_PUNCTUATION:
        return "PUNCTUATION";
    case TOKEN_KEYWORD:
        return "KEYWORD";
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}
