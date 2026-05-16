#include "standardizer.h"

#include <string.h>

static ASTNode *standardize_node(ASTNode *node);
static ASTNode *copy_node(const char *label);
static ASTNode *standardize_definition(ASTNode *node);
static ASTNode *standardize_fcn_form(ASTNode *node);
static ASTNode *standardize_lambda(ASTNode *params_node, ASTNode *body);
static ASTNode *standardize_at(ASTNode *node);
static ASTNode *standardize_let(ASTNode *definition, ASTNode *body);
static ASTNode *standardize_rec_definition(ASTNode *definition);
static ASTNode *make_lambda_chain(ASTNode *param, ASTNode *body);
static void append_standardized_children(ASTNode *target, ASTNode *source);

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
        return standardize_let(node->first_child->next_sibling, node->first_child);
    }

    if (strcmp(node->label, "within") == 0) {
        ASTNode *left = standardize_definition(node->first_child);
        ASTNode *right = standardize_definition(node->first_child->next_sibling);

        if (left != NULL && right != NULL && strcmp(left->label, "=") == 0 &&
            strcmp(right->label, "=") == 0 && left->first_child != NULL &&
            left->first_child->next_sibling != NULL && right->first_child != NULL &&
            right->first_child->next_sibling != NULL) {
            ASTNode *def = copy_node("=");
            ASTNode *gamma = copy_node("gamma");
            ASTNode *lambda = copy_node("lambda");

            add_child(lambda, standardize_node(left->first_child));
            add_child(lambda, standardize_node(right->first_child->next_sibling));
            add_child(gamma, lambda);
            add_child(gamma, standardize_node(left->first_child->next_sibling));
            add_child(def, standardize_node(right->first_child));
            add_child(def, gamma);
            return def;
        }

        result = copy_node("within");
        add_child(result, left);
        add_child(result, right);
        return result;
    }

    if (strcmp(node->label, "fcn_form") == 0) {
        return standardize_fcn_form(node);
    }

    if (strcmp(node->label, "lambda") == 0) {
        if (node->first_child != NULL && strcmp(node->first_child->label, "params") == 0) {
            return standardize_lambda(node->first_child, node->first_child->next_sibling);
        }

        result = copy_node("lambda");
        add_child(result, standardize_node(node->first_child));
        add_child(result, standardize_node(node->first_child != NULL ? node->first_child->next_sibling : NULL));
        return result;
    }

    if (strcmp(node->label, "@") == 0) {
        return standardize_at(node);
    }

    if (strcmp(node->label, "rec") == 0) {
        return standardize_rec_definition(standardize_definition(node->first_child));
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
        return standardize_let(node->first_child, node->first_child->next_sibling);
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
        strcmp(node->label, "Vl") == 0 || strcmp(node->label, "()") == 0) {
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
        return standardize_rec_definition(standardize_definition(node->first_child));
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
    ASTNode *first_param = params_node;

    if (params_node != NULL && strcmp(params_node->label, "params") == 0) {
        first_param = params_node->first_child;
    }

    return make_lambda_chain(first_param, standardize_node(body));
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

static ASTNode *standardize_let(ASTNode *definition, ASTNode *body)
{
    ASTNode *std_definition = standardize_definition(definition);
    ASTNode *gamma = NULL;
    ASTNode *lambda = NULL;

    if (std_definition == NULL || strcmp(std_definition->label, "=") != 0 ||
        std_definition->first_child == NULL ||
        std_definition->first_child->next_sibling == NULL) {
        ASTNode *fallback = copy_node("let");
        add_child(fallback, std_definition);
        add_child(fallback, standardize_node(body));
        return fallback;
    }

    gamma = copy_node("gamma");
    lambda = copy_node("lambda");
    add_child(lambda, standardize_node(std_definition->first_child));
    add_child(lambda, standardize_node(body));
    add_child(gamma, lambda);
    add_child(gamma, standardize_node(std_definition->first_child->next_sibling));
    return gamma;
}

static ASTNode *standardize_rec_definition(ASTNode *definition)
{
    ASTNode *result = NULL;
    ASTNode *gamma = NULL;
    ASTNode *lambda = NULL;

    if (definition == NULL || strcmp(definition->label, "=") != 0 ||
        definition->first_child == NULL ||
        definition->first_child->next_sibling == NULL) {
        result = copy_node("rec");
        add_child(result, definition);
        return result;
    }

    result = copy_node("=");
    gamma = copy_node("gamma");
    lambda = copy_node("lambda");

    add_child(lambda, standardize_node(definition->first_child));
    add_child(lambda, standardize_node(definition->first_child->next_sibling));
    add_child(gamma, copy_node("Y*"));
    add_child(gamma, lambda);
    add_child(result, standardize_node(definition->first_child));
    add_child(result, gamma);
    return result;
}

static ASTNode *make_lambda_chain(ASTNode *param, ASTNode *body)
{
    ASTNode *lambda = copy_node("lambda");

    if (param == NULL) {
        add_child(lambda, copy_node("()"));
        add_child(lambda, body);
        return lambda;
    }

    add_child(lambda, standardize_node(param));
    if (param->next_sibling != NULL) {
        add_child(lambda, make_lambda_chain(param->next_sibling, body));
    } else {
        add_child(lambda, body);
    }

    return lambda;
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
