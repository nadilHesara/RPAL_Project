#ifndef AST_H
#define AST_H

typedef enum AstNodeType {
    AST_NODE_EMPTY = 0
} AstNodeType;

typedef struct AstNode {
    AstNodeType type;
    char *value;
    struct AstNode *first_child;
    struct AstNode *next_sibling;
} AstNode;

AstNode *ast_node_create(AstNodeType type, const char *value);
void ast_node_destroy(AstNode *node);

#endif
