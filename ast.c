#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *duplicate_string(const char *text)
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

ASTNode *create_ast_node(const char *label)
{
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (node == NULL) {
        return NULL;
    }

    node->label = duplicate_string(label);
    node->first_child = NULL;
    node->next_sibling = NULL;

    if (label != NULL && node->label == NULL) {
        free(node);
        return NULL;
    }

    return node;
}

void add_child(ASTNode *parent, ASTNode *child)
{
    ASTNode *current = NULL;

    if (parent == NULL || child == NULL) {
        return;
    }

    /* Add as the first child, or append after the existing children. */
    if (parent->first_child == NULL) {
        parent->first_child = child;
        return;
    }

    current = parent->first_child;
    while (current->next_sibling != NULL) {
        current = current->next_sibling;
    }

    current->next_sibling = child;
}

void add_sibling(ASTNode *node, ASTNode *sibling)
{
    ASTNode *current = node;

    if (node == NULL || sibling == NULL) {
        return;
    }

    /* Append the sibling at the end of this node's sibling chain. */
    while (current->next_sibling != NULL) {
        current = current->next_sibling;
    }

    current->next_sibling = sibling;
}

void print_ast(ASTNode *root, int depth)
{
    int index = 0;
    ASTNode *child = NULL;

    if (root == NULL) {
        return;
    }

    for (index = 0; index < depth; index++) {
        putchar('.');
    }

    printf("%s\n", root->label != NULL ? root->label : "");

    child = root->first_child;
    while (child != NULL) {
        print_ast(child, depth + 1);
        child = child->next_sibling;
    }
}

void free_ast(ASTNode *root)
{
    ASTNode *child = NULL;

    if (root == NULL) {
        return;
    }

    /*
     * Free each child chain recursively before freeing this node.
     * Save next_sibling first because free_ast destroys the current child.
     */
    child = root->first_child;
    while (child != NULL) {
        ASTNode *next = child->next_sibling;
        free_ast(child);
        child = next;
    }

    free(root->label);
    free(root);
}
