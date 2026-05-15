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
    fprintf(stderr, "Usage: %s [-tokens] file_name\n", program_name);
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

int main(int argc, char *argv[])
{
    char *source = NULL;
    const char *file_name = NULL;
    int debug_tokens = 0;

    if (argc == 2)
    {
        file_name = argv[1];
    }
    else if (argc == 3 && strcmp(argv[1], "-tokens") == 0)
    {
        debug_tokens = 1;
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

    /*
     * Future interpreter pipeline:
     *   1. Tokenize source with lexer.
     *   2. Build AST with parser.
     *   3. Standardize AST.
     *   4. Evaluate with CSE machine.
     *
     * For the current milestone, only echo the input file.
     */
    printf("%s", source);

    free(source);
    return EXIT_SUCCESS;
}
