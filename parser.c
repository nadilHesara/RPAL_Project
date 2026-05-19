#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parser_advance(Parser *parser);
static int parser_check(Parser *parser, TokenType type, const char *lexeme);
static int parser_check_identifier(Parser *parser);
static int parser_check_literal(Parser *parser);
static int parser_starts_rn(Parser *parser);
static ASTNode *parser_error(Parser *parser, const char *message);
static ASTNode *parse_E(Parser *parser);
static ASTNode *parse_Ew(Parser *parser);
static ASTNode *parse_T(Parser *parser);
static ASTNode *parse_Ta(Parser *parser);
static ASTNode *parse_Tc(Parser *parser);
static ASTNode *parse_B(Parser *parser);
static ASTNode *parse_Bt(Parser *parser);
static ASTNode *parse_Bs(Parser *parser);
static ASTNode *parse_Bp(Parser *parser);
static ASTNode *parse_A(Parser *parser);
static ASTNode *parse_At(Parser *parser);
static ASTNode *parse_Af(Parser *parser);
static ASTNode *parse_Ap(Parser *parser);
static ASTNode *parse_R(Parser *parser);
static ASTNode *parse_Rn(Parser *parser);
static ASTNode *parse_D(Parser *parser);
static ASTNode *parse_Da(Parser *parser);
static ASTNode *parse_Dr(Parser *parser);
static ASTNode *parse_Db(Parser *parser);
static ASTNode *parse_Vl(Parser *parser);
static ASTNode *parse_Vb(Parser *parser);
static ASTNode *make_operator_node(const char *label, ASTNode *left, ASTNode *right);
static ASTNode *make_unary_node(const char *label, ASTNode *child);
static ASTNode *make_leaf_from_token(Token *token);
static ASTNode *make_list_node(const char *label);
static void list_append(ASTNode *list, ASTNode *child);
static int is_comparison_operator(const char *lexeme);
static int is_binary_add_operator(const char *lexeme);
static int is_binary_mul_operator(const char *lexeme);

void parser_init(Parser *parser, Lexer *lexer)
{
    if (parser == NULL) {
        return;
    }

    parser->lexer = lexer;
    parser->has_current = 0;
    parser->had_error = 0;
    parser->current.type = TOKEN_EOF;
    parser->current.lexeme = NULL;
    parser->current.line = 0;
    parser->current.column = 0;
}

ASTNode *parser_parse(Parser *parser)
{
    ASTNode *root = NULL;

    if (parser == NULL || parser->lexer == NULL) {
        return NULL;
    }

    parser_advance(parser);
    root = parse_E(parser);
    if (root == NULL) {
        if (parser->has_current) {
            token_destroy(&parser->current);
            parser->has_current = 0;
        }
        return NULL;
    }

    if (!parser_check(parser, TOKEN_EOF, NULL)) {
        parser_error(parser, "unexpected trailing input");
        free_ast(root);
        root = NULL;
    }

    if (parser->has_current) {
        token_destroy(&parser->current);
        parser->has_current = 0;
    }

    return root;
}

static void parser_advance(Parser *parser)
{
    if (parser == NULL || parser->lexer == NULL) {
        return;
    }

    if (parser->has_current) {
        token_destroy(&parser->current);
    }

    parser->current = lexer_next_token(parser->lexer);
    parser->has_current = 1;
}

static int parser_check(Parser *parser, TokenType type, const char *lexeme)
{
    if (parser == NULL || !parser->has_current) {
        return 0;
    }

    if (parser->current.type != type) {
        return 0;
    }

    if (lexeme == NULL) {
        return 1;
    }

    return parser->current.lexeme != NULL && strcmp(parser->current.lexeme, lexeme) == 0;
}

static int parser_check_identifier(Parser *parser)
{
    return parser_check(parser, TOKEN_IDENTIFIER, NULL);
}

static int parser_check_literal(Parser *parser)
{
    return parser_check_identifier(parser) ||
           parser_check(parser, TOKEN_INTEGER, NULL) ||
           parser_check(parser, TOKEN_STRING, NULL) ||
           parser_check(parser, TOKEN_KEYWORD, "true") ||
           parser_check(parser, TOKEN_KEYWORD, "false") ||
           parser_check(parser, TOKEN_KEYWORD, "nil") ||
           parser_check(parser, TOKEN_KEYWORD, "dummy");
}

static int parser_starts_rn(Parser *parser)
{
    return parser_check_literal(parser) ||
           parser_check(parser, TOKEN_PUNCTUATION, "(");
}

static ASTNode *parser_error(Parser *parser, const char *message)
{
    if (parser != NULL) {
        parser->had_error = 1;
        if (parser->has_current && parser->current.type != TOKEN_EOF) {
            fprintf(stderr, "Parser error at %lu:%lu: %s near '%s'\n",
                    parser->current.line,
                    parser->current.column,
                    message,
                    parser->current.lexeme != NULL ? parser->current.lexeme : token_type_name(parser->current.type));
        } else {
            fprintf(stderr, "Parser error: %s\n", message);
        }
    }

    return NULL;
}

static ASTNode *parse_E(Parser *parser)
{
    ASTNode *node = NULL;

    if (parser_check(parser, TOKEN_KEYWORD, "let")) {
        ASTNode *defs = NULL;
        ASTNode *body = NULL;

        parser_advance(parser);
        defs = parse_D(parser);
        if (defs == NULL || !parser_check(parser, TOKEN_KEYWORD, "in")) {
            return parser_error(parser, "expected 'in' after let definition");
        }
        parser_advance(parser);
        body = parse_E(parser);
        if (body == NULL) {
            return NULL;
        }

        node = create_ast_node("let");
        add_child(node, defs);
        add_child(node, body);
        return node;
    }

    if (parser_check(parser, TOKEN_KEYWORD, "fn")) {
        ASTNode *params = make_list_node("params");
        ASTNode *body = NULL;

        parser_advance(parser);
        while (parser_check_identifier(parser) || parser_check(parser, TOKEN_PUNCTUATION, "(") ) {
            ASTNode *vb = parse_Vb(parser);
            if (vb == NULL) {
                free_ast(params);
                return NULL;
            }
            list_append(params, vb);
        }

        if (!parser_check(parser, TOKEN_OPERATOR, ".") && !parser_check(parser, TOKEN_PUNCTUATION, ".")) {
            /* The lexer emits '.' as an operator token. */
            return parser_error(parser, "expected '.' after fn parameters");
        }

        parser_advance(parser);
        body = parse_E(parser);
        if (body == NULL) {
            free_ast(params);
            return NULL;
        }

        node = create_ast_node("lambda");
        add_child(node, params);
        add_child(node, body);
        return node;
    }

    return parse_Ew(parser);
}

static ASTNode *parse_Ew(Parser *parser)
{
    ASTNode *left = parse_T(parser);
    if (left == NULL) {
        return NULL;
    }

    if (parser_check(parser, TOKEN_KEYWORD, "where")) {
        ASTNode *defs = NULL;
        ASTNode *node = NULL;

        parser_advance(parser);
        defs = parse_D(parser);
        if (defs == NULL) {
            free_ast(left);
            return NULL;
        }

        node = create_ast_node("where");
        add_child(node, left);
        add_child(node, defs);
        return node;
    }

    return left;
}

static ASTNode *parse_T(Parser *parser)
{
    ASTNode *first = parse_Ta(parser);
    ASTNode *tuple = NULL;

    if (first == NULL) {
        return NULL;
    }

    if (!parser_check(parser, TOKEN_PUNCTUATION, ",")) {
        return first;
    }

    tuple = make_list_node("tau");
    list_append(tuple, first);

    while (parser_check(parser, TOKEN_PUNCTUATION, ",")) {
        ASTNode *next = NULL;
        parser_advance(parser);
        next = parse_Ta(parser);
        if (next == NULL) {
            free_ast(tuple);
            return NULL;
        }
        list_append(tuple, next);
    }

    return tuple;
}

static ASTNode *parse_Ta(Parser *parser)
{
    ASTNode *node = parse_Tc(parser);

    if (node == NULL) {
        return NULL;
    }

    while (parser_check(parser, TOKEN_OPERATOR, "aug")) {
        ASTNode *rhs = NULL;
        ASTNode *aug = NULL;
        parser_advance(parser);
        rhs = parse_Tc(parser);
        if (rhs == NULL) {
            free_ast(node);
            return NULL;
        }
        aug = create_ast_node("aug");
        add_child(aug, node);
        add_child(aug, rhs);
        node = aug;
    }

    return node;
}

static ASTNode *parse_Tc(Parser *parser)
{
    ASTNode *cond = parse_B(parser);

    if (cond == NULL) {
        return NULL;
    }

    if (!parser_check(parser, TOKEN_OPERATOR, "->")) {
        return cond;
    }

    parser_advance(parser);
    {
        ASTNode *then_branch = parse_Tc(parser);
        ASTNode *else_branch = NULL;
        ASTNode *node = NULL;

        if (then_branch == NULL || !parser_check(parser, TOKEN_OPERATOR, "|")) {
            free_ast(cond);
            free_ast(then_branch);
            return parser_error(parser, "expected '|' in conditional expression");
        }

        parser_advance(parser);
        else_branch = parse_Tc(parser);
        if (else_branch == NULL) {
            free_ast(cond);
            free_ast(then_branch);
            return NULL;
        }

        node = create_ast_node("->");
        add_child(node, cond);
        add_child(node, then_branch);
        add_child(node, else_branch);
        return node;
    }
}

static ASTNode *parse_B(Parser *parser)
{
    ASTNode *node = parse_Bt(parser);

    if (node == NULL) {
            return NULL;
    }

    while (parser_check(parser, TOKEN_OPERATOR, "or")) {
        ASTNode *rhs = NULL;
        ASTNode *or_node = NULL;
        parser_advance(parser);
        rhs = parse_Bt(parser);
        if (rhs == NULL) {
            free_ast(node);
            return NULL;
        }
        or_node = create_ast_node("or");
        add_child(or_node, node);
        add_child(or_node, rhs);
        node = or_node;
    }

    return node;
}

static ASTNode *parse_Bt(Parser *parser)
{
    ASTNode *node = parse_Bs(parser);

    if (node == NULL) {
        return NULL;
    }

    while (parser_check(parser, TOKEN_OPERATOR, "&")) {
        ASTNode *rhs = NULL;
        ASTNode *and_node = NULL;
        parser_advance(parser);
        rhs = parse_Bs(parser);
        if (rhs == NULL) {
            free_ast(node);
            return NULL;
        }
        and_node = create_ast_node("&");
        add_child(and_node, node);
        add_child(and_node, rhs);
        node = and_node;
    }

    return node;
}

static ASTNode *parse_Bs(Parser *parser)
{
    if (parser_check(parser, TOKEN_OPERATOR, "not")) {
        ASTNode *child = NULL;
        ASTNode *node = NULL;
        parser_advance(parser);
        child = parse_Bp(parser);
        if (child == NULL) {
            return NULL;
        }
        node = create_ast_node("not");
        add_child(node, child);
        return node;
    }

    return parse_Bp(parser);
}

static ASTNode *parse_Bp(Parser *parser)
{
    ASTNode *left = parse_A(parser);

    if (left == NULL) {
        return NULL;
    }

    if (parser_check(parser, TOKEN_OPERATOR, "gr") || parser_check(parser, TOKEN_OPERATOR, ">") ||
        parser_check(parser, TOKEN_OPERATOR, "ge") || parser_check(parser, TOKEN_OPERATOR, ">=") ||
        parser_check(parser, TOKEN_OPERATOR, "ls") || parser_check(parser, TOKEN_OPERATOR, "<") ||
        parser_check(parser, TOKEN_OPERATOR, "le") || parser_check(parser, TOKEN_OPERATOR, "<=") ||
        parser_check(parser, TOKEN_OPERATOR, "eq") || parser_check(parser, TOKEN_OPERATOR, "ne")) {
        ASTNode *right = NULL;
        ASTNode *node = NULL;
        const char *label = parser->current.lexeme;

        node = create_ast_node(label);
        if (node == NULL) {
            free_ast(left);
            return NULL;
        }

        parser_advance(parser);
        right = parse_A(parser);
        if (right == NULL) {
            free_ast(left);
            free_ast(node);
            return NULL;
        }
        add_child(node, left);
        add_child(node, right);
        return node;
    }

    return left;
}

static ASTNode *parse_A(Parser *parser)
{
    if (parser_check(parser, TOKEN_OPERATOR, "+") || parser_check(parser, TOKEN_OPERATOR, "-")) {
        ASTNode *child = NULL;
        ASTNode *node = NULL;
        const char *op = parser->current.lexeme;

        node = create_ast_node(strcmp(op, "-") == 0 ? "neg" : "uplus");
        if (node == NULL) {
            return NULL;
        }

        parser_advance(parser);
        child = parse_At(parser);
        if (child == NULL) {
            free_ast(node);
            return NULL;
        }
        add_child(node, child);
        return node;
    }

    {
        ASTNode *node = parse_At(parser);
        if (node == NULL) {
            return NULL;
        }

        while (parser_check(parser, TOKEN_OPERATOR, "+") || parser_check(parser, TOKEN_OPERATOR, "-")) {
            const char *op = parser->current.lexeme;
            ASTNode *rhs = NULL;
            ASTNode *bin = NULL;

            bin = create_ast_node(op);
            if (bin == NULL) {
                free_ast(node);
                return NULL;
            }

            parser_advance(parser);
            rhs = parse_At(parser);
            if (rhs == NULL) {
                free_ast(node);
                free_ast(bin);
                return NULL;
            }
            add_child(bin, node);
            add_child(bin, rhs);
            node = bin;
        }

        return node;
    }
}

static ASTNode *parse_At(Parser *parser)
{
    ASTNode *node = parse_Af(parser);

    if (node == NULL) {
        return NULL;
    }

    while (parser_check(parser, TOKEN_OPERATOR, "*") || parser_check(parser, TOKEN_OPERATOR, "/")) {
        const char *op = parser->current.lexeme;
        ASTNode *rhs = NULL;
        ASTNode *bin = NULL;

        bin = create_ast_node(op);
        if (bin == NULL) {
            free_ast(node);
            return NULL;
        }

        parser_advance(parser);
        rhs = parse_Af(parser);
        if (rhs == NULL) {
            free_ast(node);
            free_ast(bin);
            return NULL;
        }
        add_child(bin, node);
        add_child(bin, rhs);
        node = bin;
    }

    return node;
}

static ASTNode *parse_Af(Parser *parser)
{
    ASTNode *left = parse_Ap(parser);

    if (left == NULL) {
        return NULL;
    }

    if (parser_check(parser, TOKEN_OPERATOR, "**")) {
        ASTNode *rhs = NULL;
        ASTNode *node = NULL;
        parser_advance(parser);
        rhs = parse_Af(parser);
        if (rhs == NULL) {
            free_ast(left);
            return NULL;
        }
        node = create_ast_node("**");
        add_child(node, left);
        add_child(node, rhs);
        return node;
    }

    return left;
}

static ASTNode *parse_Ap(Parser *parser)
{
    ASTNode *node = parse_R(parser);

    if (node == NULL) {
        return NULL;
    }

    while (parser_check(parser, TOKEN_OPERATOR, "@")) {
        ASTNode *ident = NULL;
        ASTNode *arg = NULL;
        ASTNode *at = NULL;
        parser_advance(parser);
        if (!parser_check_identifier(parser)) {
            free_ast(node);
            return parser_error(parser, "expected identifier after '@'");
        }
        ident = make_leaf_from_token(&parser->current);
        parser_advance(parser);
        arg = parse_R(parser);
        if (arg == NULL || ident == NULL) {
            free_ast(node);
            free_ast(ident);
            free_ast(arg);
            return NULL;
        }
        at = create_ast_node("@");
        add_child(at, node);
        add_child(at, ident);
        add_child(at, arg);
        node = at;
    }

    return node;
}

static ASTNode *parse_R(Parser *parser)
{
    ASTNode *node = parse_Rn(parser);

    if (node == NULL) {
        return NULL;
    }

    while (parser_starts_rn(parser)) {
        ASTNode *arg = parse_Rn(parser);
        ASTNode *gamma = NULL;
        if (arg == NULL) {
            free_ast(node);
            return NULL;
        }
        gamma = create_ast_node("gamma");
        add_child(gamma, node);
        add_child(gamma, arg);
        node = gamma;
    }

    return node;
}

static ASTNode *parse_Rn(Parser *parser)
{
    ASTNode *node = NULL;

    if (parser_check_identifier(parser) ||
        parser_check(parser, TOKEN_INTEGER, NULL) ||
        parser_check(parser, TOKEN_STRING, NULL) ||
        parser_check(parser, TOKEN_KEYWORD, "true") ||
        parser_check(parser, TOKEN_KEYWORD, "false") ||
        parser_check(parser, TOKEN_KEYWORD, "nil") ||
        parser_check(parser, TOKEN_KEYWORD, "dummy")) {
        node = make_leaf_from_token(&parser->current);
        parser_advance(parser);
        return node;
    }

    if (parser_check(parser, TOKEN_PUNCTUATION, "(")) {
        ASTNode *expr = NULL;
        parser_advance(parser);
        expr = parse_E(parser);
        if (expr == NULL || !parser_check(parser, TOKEN_PUNCTUATION, ")")) {
            free_ast(expr);
            return parser_error(parser, "expected ')' after expression");
        }
        parser_advance(parser);
        return expr;
    }

    return parser_error(parser, "expected expression atom");
}

static ASTNode *parse_D(Parser *parser)
{
    ASTNode *left = parse_Da(parser);

    if (left == NULL) {
        return NULL;
    }

    if (parser_check(parser, TOKEN_KEYWORD, "within")) {
        ASTNode *right = NULL;
        ASTNode *node = NULL;
        parser_advance(parser);
        right = parse_D(parser);
        if (right == NULL) {
            free_ast(left);
            return NULL;
        }
        node = create_ast_node("within");
        add_child(node, left);
        add_child(node, right);
        return node;
    }

    return left;
}

static ASTNode *parse_Da(Parser *parser)
{
    ASTNode *node = parse_Dr(parser);

    if (node == NULL) {
        return NULL;
    }

    if (!parser_check(parser, TOKEN_OPERATOR, "and")) {
        return node;
    }

    {
        ASTNode *and_node = make_list_node("and");
        list_append(and_node, node);

        while (parser_check(parser, TOKEN_OPERATOR, "and")) {
            ASTNode *next = NULL;
            parser_advance(parser);
            next = parse_Dr(parser);
            if (next == NULL) {
                free_ast(and_node);
                return NULL;
            }
            list_append(and_node, next);
        }

        return and_node;
    }
}

static ASTNode *parse_Dr(Parser *parser)
{
    if (parser_check(parser, TOKEN_KEYWORD, "rec")) {
        ASTNode *child = NULL;
        ASTNode *node = NULL;
        parser_advance(parser);
        child = parse_Db(parser);
        if (child == NULL) {
            return NULL;
        }
        node = create_ast_node("rec");
        add_child(node, child);
        return node;
    }

    return parse_Db(parser);
}

static ASTNode *parse_Db(Parser *parser)
{
    if (parser_check(parser, TOKEN_PUNCTUATION, "(")) {
        parser_advance(parser);
        if (parser_check(parser, TOKEN_PUNCTUATION, ")")) {
            ASTNode *node = create_ast_node("()");
            parser_advance(parser);
            return node;
        }

        {
            ASTNode *inner = parse_D(parser);
            if (inner == NULL || !parser_check(parser, TOKEN_PUNCTUATION, ")")) {
                free_ast(inner);
                return parser_error(parser, "expected ')' after definition");
            }
            parser_advance(parser);
            return inner;
        }
    }

    if (parser_check_identifier(parser)) {
        ASTNode *name = make_leaf_from_token(&parser->current);
        ASTNode *left = NULL;
        ASTNode *right = NULL;

        parser_advance(parser);

        if (parser_check(parser, TOKEN_OPERATOR, "=") || parser_check(parser, TOKEN_PUNCTUATION, "=")) {
            ASTNode *node = create_ast_node("=");
            parser_advance(parser);
            right = parse_E(parser);
            if (right == NULL) {
                free_ast(name);
                free_ast(node);
                return NULL;
            }
            add_child(node, name);
            add_child(node, right);
            return node;
        }

        if (parser_check(parser, TOKEN_PUNCTUATION, ",")) {
            ASTNode *vars = make_list_node("Vl");
            list_append(vars, name);
            while (parser_check(parser, TOKEN_PUNCTUATION, ",")) {
                parser_advance(parser);
                if (!parser_check_identifier(parser)) {
                    free_ast(vars);
                    return parser_error(parser, "expected identifier in variable list");
                }
                list_append(vars, make_leaf_from_token(&parser->current));
                parser_advance(parser);
            }

            if (!parser_check(parser, TOKEN_OPERATOR, "=") && !parser_check(parser, TOKEN_PUNCTUATION, "=")) {
                free_ast(vars);
                return parser_error(parser, "expected '=' after variable list");
            }
            parser_advance(parser);
            right = parse_E(parser);
            if (right == NULL) {
                free_ast(vars);
                return NULL;
            }
            left = create_ast_node("=");
            add_child(left, vars);
            add_child(left, right);
            return left;
        }

        if (parser_check_identifier(parser) || parser_check(parser, TOKEN_PUNCTUATION, "(") || parser_check(parser, TOKEN_PUNCTUATION, "()")) {
            ASTNode *params = make_list_node("params");
            ASTNode *fcn = NULL;

            list_append(params, name);
            while (parser_check_identifier(parser) || parser_check(parser, TOKEN_PUNCTUATION, "(") ) {
                ASTNode *vb = parse_Vb(parser);
                if (vb == NULL) {
                    free_ast(params);
                    return NULL;
                }
                list_append(params, vb);
            }

            if (!parser_check(parser, TOKEN_OPERATOR, "=") && !parser_check(parser, TOKEN_PUNCTUATION, "=")) {
                free_ast(params);
                return parser_error(parser, "expected '=' after function form");
            }
            parser_advance(parser);
            right = parse_E(parser);
            if (right == NULL) {
                free_ast(params);
                return NULL;
            }
            fcn = create_ast_node("function_form");
            add_child(fcn, params);
            add_child(fcn, right);
            return fcn;
        }

        return parser_error(parser, "expected '=', ',' or function parameters after identifier");
    }

    return parser_error(parser, "expected definition");
}

static ASTNode *parse_Vl(Parser *parser)
{
    ASTNode *vars = make_list_node("Vl");

    if (!parser_check_identifier(parser)) {
        free_ast(vars);
        return parser_error(parser, "expected identifier");
    }

    list_append(vars, make_leaf_from_token(&parser->current));
    parser_advance(parser);

    while (parser_check(parser, TOKEN_PUNCTUATION, ",")) {
        parser_advance(parser);
        if (!parser_check_identifier(parser)) {
            free_ast(vars);
            return parser_error(parser, "expected identifier after ','");
        }
        list_append(vars, make_leaf_from_token(&parser->current));
        parser_advance(parser);
    }

    return vars;
}

static ASTNode *parse_Vb(Parser *parser)
{
    if (parser_check_identifier(parser)) {
        ASTNode *leaf = make_leaf_from_token(&parser->current);
        parser_advance(parser);
        return leaf;
    }

    if (parser_check(parser, TOKEN_PUNCTUATION, "(")) {
        ASTNode *node = NULL;
        parser_advance(parser);
        if (parser_check(parser, TOKEN_PUNCTUATION, ")")) {
            parser_advance(parser);
            return create_ast_node("()");
        }
        node = parse_Vl(parser);
        if (node == NULL || !parser_check(parser, TOKEN_PUNCTUATION, ")")) {
            free_ast(node);
            return parser_error(parser, "expected ')' in parameter list");
        }
        parser_advance(parser);
        return node;
    }

    return parser_error(parser, "expected variable binding");
}

static ASTNode *make_operator_node(const char *label, ASTNode *left, ASTNode *right)
{
    ASTNode *node = create_ast_node(label);
    if (node == NULL) {
        free_ast(left);
        free_ast(right);
        return NULL;
    }
    add_child(node, left);
    add_child(node, right);
    return node;
}

static ASTNode *make_unary_node(const char *label, ASTNode *child)
{
    ASTNode *node = create_ast_node(label);
    if (node == NULL) {
        free_ast(child);
        return NULL;
    }
    add_child(node, child);
    return node;
}

static ASTNode *make_leaf_from_token(Token *token)
{
    if (token == NULL) {
        return NULL;
    }

    return create_ast_node(token->lexeme != NULL ? token->lexeme : token_type_name(token->type));
}

static ASTNode *make_list_node(const char *label)
{
    return create_ast_node(label);
}

static void list_append(ASTNode *list, ASTNode *child)
{
    add_child(list, child);
}

static int is_comparison_operator(const char *lexeme)
{
    return lexeme != NULL && (
        strcmp(lexeme, "gr") == 0 || strcmp(lexeme, ">") == 0 ||
        strcmp(lexeme, "ge") == 0 || strcmp(lexeme, ">=") == 0 ||
        strcmp(lexeme, "ls") == 0 || strcmp(lexeme, "<") == 0 ||
        strcmp(lexeme, "le") == 0 || strcmp(lexeme, "<=") == 0 ||
        strcmp(lexeme, "eq") == 0 || strcmp(lexeme, "ne") == 0);
}

static int is_binary_add_operator(const char *lexeme)
{
    return lexeme != NULL && (strcmp(lexeme, "+") == 0 || strcmp(lexeme, "-") == 0);
}

static int is_binary_mul_operator(const char *lexeme)
{
    return lexeme != NULL && (strcmp(lexeme, "*") == 0 || strcmp(lexeme, "/") == 0);
}
