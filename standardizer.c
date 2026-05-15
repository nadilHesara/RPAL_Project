#include "standardizer.h"

#include <string.h>

static ASTNode *standardize_node(ASTNode *node);
static ASTNode *copy_node(const char *label);
static ASTNode *standardize_children_to_list(ASTNode *source, const char *label);
static ASTNode *standardize_definition(ASTNode *node);
static ASTNode *standardize_fcn_form(ASTNode *node);
static ASTNode *standardize_lambda(ASTNode *params_node, ASTNode *body);
static ASTNode *standardize_at(ASTNode *node);
static void append_standardized_children(ASTNode *target, ASTNode *source);
static void append_param_list(ASTNode *target, ASTNode *node);
static ASTNode *make_tuple_from_list(ASTNode *list_node);

ASTNode *standardize_ast(ASTNode *root)
{
    return standardize_node(root);
}

static ASTNode *standardize_node(ASTNode *node)
{
    ASTNode *result = NULL;
    ASTNode *child = NULL;

    if (node == NULL) {
        return NULL;
    }

    if (node->first_child == NULL) {
        return copy_node(node->label);
    }

    if (strcmp(node->label, "where") == 0) {
        ASTNode *defs = standardize_node(node->first_child->next_sibling);
        ASTNode *expr = standardize_node(node->first_child);
        result = copy_node("let");
        add_child(result, defs);
        add_child(result, expr);
        return result;
    }

    if (strcmp(node->label, "within") == 0) {
        ASTNode *left = standardize_definition(node->first_child);
        ASTNode *right = standardize_definition(node->first_child->next_sibling);
        result = copy_node("let");
        add_child(result, left);
        add_child(result, right);
        return result;
    }

    if (strcmp(node->label, "fcn_form") == 0) {
        return standardize_fcn_form(node);
    }

    if (strcmp(node->label, "lambda") == 0) {
        return standardize_lambda(node->first_child, node->first_child->next_sibling);
    }

    if (strcmp(node->label, "@") == 0) {
        return standardize_at(node);
    }

    if (strcmp(node->label, "rec") == 0) {
        result = copy_node("rec");
        add_child(result, standardize_definition(node->first_child));
        return result;
    }

    if (strcmp(node->label, "and") == 0) {
        ASTNode *vars = copy_node("tau");
        ASTNode *exprs = copy_node("tau");

        for (child = node->first_child; child != NULL; child = child->next_sibling) {
            ASTNode *std_def = standardize_definition(child);
            if (std_def != NULL && strcmp(std_def->label, "=") == 0 && std_def->first_child != NULL && std_def->first_child->next_sibling != NULL) {
                add_child(vars, standardize_node(std_def->first_child));
                add_child(exprs, standardize_node(std_def->first_child->next_sibling));
            } else {
                add_child(vars, standardize_node(std_def));
                add_child(exprs, standardize_node(std_def));
            }
        }

        result = copy_node("=");
        add_child(result, vars);
        add_child(result, exprs);
        return result;
    }

    if (strcmp(node->label, "let") == 0) {
        result = copy_node("let");
        append_standardized_children(result, node);
        return result;
    }

    if (strcmp(node->label, "tau") == 0 || strcmp(node->label, "gamma") == 0 ||
        strcmp(node->label, "aug") == 0 || strcmp(node->label, "+") == 0 ||
        strcmp(node->label, "-") == 0 || strcmp(node->label, "*") == 0 ||
        strcmp(node->label, "/") == 0 || strcmp(node->label, "**") == 0 ||
        strcmp(node->label, "neg") == 0 || strcmp(node->label, "uplus") == 0 ||
        strcmp(node->label, "or") == 0 || strcmp(node->label, "&") == 0 ||
        strcmp(node->label, "not") == 0 || strcmp(node->label, "gr") == 0 ||
        strcmp(node->label, "ge") == 0 || strcmp(node->label, "ls") == 0 ||
        strcmp(node->label, "le") == 0 || strcmp(node->label, "eq") == 0 ||
        strcmp(node->label, "ne") == 0 || strcmp(node->label, "->") == 0 ||
        strcmp(node->label, "Vl") == 0 || strcmp(node->label, "params") == 0 ||
        strcmp(node->label, "()") == 0) {
        result = copy_node(node->label);
        append_standardized_children(result, node);
        return result;
    }

    result = copy_node(node->label);
    append_standardized_children(result, node);
    return result;
}

static ASTNode *standardize_definition(ASTNode *node)
{
    if (node == NULL) {
        return NULL;
    }

    if (strcmp(node->label, "fcn_form") == 0) {
        return standardize_fcn_form(node);
    }

    if (strcmp(node->label, "rec") == 0) {
        ASTNode *result = copy_node("rec");
        add_child(result, standardize_definition(node->first_child));
        return result;
    }

    if (strcmp(node->label, "and") == 0) {
        return standardize_node(node);
    }

    return standardize_node(node);
}

static ASTNode *standardize_fcn_form(ASTNode *node)
{
    ASTNode *params = NULL;
    ASTNode *body = NULL;
    ASTNode *name = NULL;
    ASTNode *lambda = NULL;
    ASTNode *def = NULL;

    if (node == NULL || node->first_child == NULL || node->first_child->next_sibling == NULL) {
        return standardize_node(node);
    }

    params = node->first_child;
    body = node->first_child->next_sibling;
    name = params->first_child;

    if (name == NULL) {
        return standardize_node(node);
    }

    lambda = standardize_lambda(params->first_child->next_sibling, body);
    def = copy_node("=");
    add_child(def, standardize_node(name));
    add_child(def, lambda);
    return def;
}

static ASTNode *standardize_lambda(ASTNode *params_node, ASTNode *body)
{
    ASTNode *lambda = copy_node("lambda");
    ASTNode *params = copy_node("params");
    ASTNode *child = NULL;

    if (params_node != NULL) {
        append_param_list(params, params_node);
    }

    add_child(lambda, params);
    add_child(lambda, standardize_node(body));
    return lambda;
}

static ASTNode *standardize_at(ASTNode *node)
{
    ASTNode *gamma_outer = copy_node("gamma");
    ASTNode *gamma_inner = copy_node("gamma");
    ASTNode *left = standardize_node(node->first_child);
    ASTNode *ident = standardize_node(node->first_child->next_sibling);
    ASTNode *arg = standardize_node(node->first_child->next_sibling->next_sibling);

    add_child(gamma_inner, ident);
    add_child(gamma_inner, left);
    add_child(gamma_outer, gamma_inner);
    add_child(gamma_outer, arg);
    return gamma_outer;
}

static ASTNode *copy_node(const char *label)
{
    return create_ast_node(label);
}

static void append_standardized_children(ASTNode *target, ASTNode *source)
{
    ASTNode *child = NULL;

    if (target == NULL || source == NULL) {
        return;
    }

    for (child = source->first_child; child != NULL; child = child->next_sibling) {
        add_child(target, standardize_node(child));
    }
}

static void append_param_list(ASTNode *target, ASTNode *node)
{
    ASTNode *child = NULL;

    if (target == NULL || node == NULL) {
        return;
    }

    if (strcmp(node->label, "Vl") == 0 || strcmp(node->label, "params") == 0 || strcmp(node->label, "tau") == 0) {
        for (child = node->first_child; child != NULL; child = child->next_sibling) {
            append_param_list(target, child);
        }
        return;
    }

    if (strcmp(node->label, "()") == 0) {
        add_child(target, copy_node("()"));
        return;
    }

    if (node->first_child != NULL) {
        for (child = node->first_child; child != NULL; child = child->next_sibling) {
            append_param_list(target, child);
        }
        return;
    }

    add_child(target, copy_node(node->label));
}
