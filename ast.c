#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_identifier_label(const char *label);
static int is_integer_label(const char *label);
static int is_string_label(const char *label);
static int is_ast_wrapper_label(const char *label);
static const char *format_ast_leaf(const char *label);

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

void print_ast_pretty(ASTNode *root, int depth)
{
    int index = 0;
    ASTNode *child = NULL;

    if (root == NULL) {
        return;
    }

    if (is_ast_wrapper_label(root->label)) {
        child = root->first_child;
        while (child != NULL) {
            print_ast_pretty(child, depth);
            child = child->next_sibling;
        }
        return;
    }

    for (index = 0; index < depth; index++) {
        putchar('.');
    }

    if (root->first_child == NULL) {
        printf("%s\n", format_ast_leaf(root->label));
    } else {
        printf("%s\n", root->label != NULL ? root->label : "");
    }

    child = root->first_child;
    while (child != NULL) {
        print_ast_pretty(child, depth + 1);
        child = child->next_sibling;
    }
}

static int is_identifier_label(const char *label)
{
    size_t index = 0;

    if (label == NULL || label[0] == '\0') {
        return 0;
    }

    if (!(label[0] == '_' || (label[0] >= 'A' && label[0] <= 'Z') ||
          (label[0] >= 'a' && label[0] <= 'z'))) {
        return 0;
    }

    for (index = 1; label[index] != '\0'; index++) {
        char value = label[index];

        if (!(value == '_' || (value >= 'A' && value <= 'Z') ||
              (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9'))) {
            return 0;
        }
    }

    return strcmp(label, "let") != 0 && strcmp(label, "in") != 0 &&
           strcmp(label, "where") != 0 && strcmp(label, "fn") != 0 &&
           strcmp(label, "within") != 0 && strcmp(label, "rec") != 0 &&
           strcmp(label, "true") != 0 && strcmp(label, "false") != 0 &&
           strcmp(label, "nil") != 0 && strcmp(label, "dummy") != 0 &&
           strcmp(label, "lambda") != 0 && strcmp(label, "gamma") != 0 &&
           strcmp(label, "tau") != 0 && strcmp(label, "aug") != 0 &&
           strcmp(label, "or") != 0 && strcmp(label, "and") != 0 &&
           strcmp(label, "not") != 0 && strcmp(label, "gr") != 0 &&
           strcmp(label, "ge") != 0 && strcmp(label, "ls") != 0 &&
           strcmp(label, "le") != 0 && strcmp(label, "eq") != 0 &&
           strcmp(label, "ne") != 0 && strcmp(label, "->") != 0 &&
           strcmp(label, "+") != 0 && strcmp(label, "-") != 0 &&
           strcmp(label, "*") != 0 && strcmp(label, "/") != 0 &&
           strcmp(label, "**") != 0 && strcmp(label, "neg") != 0 &&
           strcmp(label, "uplus") != 0 && strcmp(label, "Vl") != 0 &&
           strcmp(label, "params") != 0 && strcmp(label, "()") != 0;
}

static int is_integer_label(const char *label)
{
    size_t index = 0;

    if (label == NULL || label[0] == '\0') {
        return 0;
    }

    for (index = 0; label[index] != '\0'; index++) {
        if (label[index] < '0' || label[index] > '9') {
            return 0;
        }
    }

    return 1;
}

static int is_string_label(const char *label)
{
    size_t length = 0;

    if (label == NULL) {
        return 0;
    }

    length = strlen(label);
    return length >= 2 && label[0] == '\'' && label[length - 1] == '\'';
}

static int is_ast_wrapper_label(const char *label)
{
    return label != NULL && (strcmp(label, "params") == 0 || strcmp(label, "Vl") == 0);
}

static const char *format_ast_leaf(const char *label)
{
    static char buffer[2048];

    if (label == NULL) {
        return "";
    }

    if (is_integer_label(label)) {
        snprintf(buffer, sizeof(buffer), "<INT:%s>", label);
        return buffer;
    }

    if (is_string_label(label)) {
        snprintf(buffer, sizeof(buffer), "<STR:%s>", label);
        return buffer;
    }

    if (is_identifier_label(label)) {
        snprintf(buffer, sizeof(buffer), "<ID:%s>", label);
        return buffer;
    }

    return label;
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
