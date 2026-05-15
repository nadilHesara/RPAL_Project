#include "parser.h"

#include <stddef.h>

void parser_init(Parser *parser, Lexer *lexer)
{
    if (parser == 0) {
        return;
    }

    parser->lexer = lexer;
}

AstNode *parser_parse(Parser *parser)
{
    (void)parser;

    /*
     * Parsing will be implemented after the RPAL lexer is complete.
     * Return an empty root node for now.
     */
    return ast_node_create(AST_NODE_EMPTY, NULL);
}
