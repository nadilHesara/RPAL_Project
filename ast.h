#ifndef AST_H
#define AST_H

typedef struct ASTNode {
    char *label;
    struct ASTNode *first_child;
    struct ASTNode *next_sibling;
} ASTNode;

ASTNode *create_ast_node(const char *label);
void add_child(ASTNode *parent, ASTNode *child);
void add_sibling(ASTNode *node, ASTNode *sibling);
void print_ast(ASTNode *root, int depth);
void print_ast_pretty(ASTNode *root, int depth);
void free_ast(ASTNode *root);

#endif
