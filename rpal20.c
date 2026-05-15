#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "cse.h"
#include "lexer.h"
#include "parser.h"
#include "standardizer.h"
#include "utils.h"

static void print_usage(const char *program_name)
{
    fprintf(stderr, "Usage: %s [-tokens|-ast|-st] file_name\n", program_name);
}

static int print_tokens(const char *source)
{
    Lexer lexer;
    Token token;
    int had_error = 0;

    lexer_init(&lexer, source);

    do
    {
        token = lexer_next_token(&lexer);
        if (token.lexeme != NULL)
        {
            printf("%lu:%lu  %-11s  %s\n", token.line, token.column,
                   token_type_name(token.type), token.lexeme);
        }
        else
        {
            printf("%lu:%lu  %s\n", token.line, token.column,
                   token_type_name(token.type));
        }

        if (token.type == TOKEN_ERROR)
        {
            had_error = 1;
        }

        token_destroy(&token);
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);

    return had_error ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int print_ast_from_source(const char *source)
{
    Lexer lexer;
    Parser parser;
    ASTNode *root = NULL;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    root = parser_parse(&parser);

    if (root == NULL)
    {
        fprintf(stderr, "Error: failed to build AST\n");
        return EXIT_FAILURE;
    }

    print_ast(root, 0);
    free_ast(root);
    return EXIT_SUCCESS;
}

static int print_standardized_tree_from_source(const char *source)
{
    Lexer lexer;
    Parser parser;
    ASTNode *ast = NULL;
    ASTNode *st = NULL;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    ast = parser_parse(&parser);
    if (ast == NULL)
    {
        fprintf(stderr, "Error: failed to parse input\n");
        return EXIT_FAILURE;
    }

    st = standardize_ast(ast);
    free_ast(ast);
    if (st == NULL)
    {
        fprintf(stderr, "Error: failed to standardize AST\n");
        return EXIT_FAILURE;
    }

    print_ast(st, 0);
    free_ast(st);
    return EXIT_SUCCESS;
}

static int execute_source(const char *source)
{
    Lexer lexer;
    Parser parser;
    ASTNode *ast = NULL;
    ASTNode *st = NULL;
    int status = EXIT_FAILURE;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    ast = parser_parse(&parser);
    if (ast == NULL)
    {
        fprintf(stderr, "Error: failed to parse input\n");
        return EXIT_FAILURE;
    }

    st = standardize_ast(ast);
    free_ast(ast);
    if (st == NULL)
    {
        fprintf(stderr, "Error: failed to standardize AST\n");
        return EXIT_FAILURE;
    }

    status = cse_evaluate(st);
    free_ast(st);
    return status == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    char *source = NULL;
    const char *file_name = NULL;
    int debug_tokens = 0;
    int debug_ast = 0;
    int debug_st = 0;

    if (argc == 2)
    {
        file_name = argv[1];
    }
    else if (argc == 3 && strcmp(argv[1], "-tokens") == 0)
    {
        debug_tokens = 1;
        file_name = argv[2];
    }
    else if (argc == 3 && strcmp(argv[1], "-ast") == 0)
    {
        debug_ast = 1;
        file_name = argv[2];
    }
    else if (argc == 3 && strcmp(argv[1], "-st") == 0)
    {
        debug_st = 1;
        file_name = argv[2];
    }
    else
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    source = read_file_contents(file_name);
    if (source == NULL)
    {
        return EXIT_FAILURE;
    }

    if (debug_tokens)
    {
        int status = print_tokens(source);
        free(source);
        return status;
    }

    if (debug_ast)
    {
        int status = print_ast_from_source(source);
        free(source);
        return status;
    }

    if (debug_st)
    {
        int status = print_standardized_tree_from_source(source);
        free(source);
        return status;
    }

    {
        int status = execute_source(source);
        free(source);
        return status;
    }
}
