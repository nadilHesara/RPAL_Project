#include "ast.h"

#include <stdlib.h>
#include <string.h>

static char *ast_strdup(const char *text)
{
    size_t length = 0;
    char *copy = NULL;

    if (text == NULL) {
        return NULL;
    }

    length = strlen(text);
    copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length + 1);
    return copy;
}

AstNode *ast_node_create(AstNodeType type, const char *value)
{
    AstNode *node = (AstNode *)malloc(sizeof(AstNode));
    if (node == NULL) {
        return NULL;
    }

    node->type = type;
    node->value = ast_strdup(value);
    node->first_child = NULL;
    node->next_sibling = NULL;

    if (value != NULL && node->value == NULL) {
        free(node);
        return NULL;
    }

    return node;
}

void ast_node_destroy(AstNode *node)
{
    AstNode *child = NULL;

    if (node == NULL) {
        return;
    }

    child = node->first_child;
    while (child != NULL) {
        AstNode *next = child->next_sibling;
        ast_node_destroy(child);
        child = next;
    }

    free(node->value);
    free(node);
}
