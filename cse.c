#include "cse.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum ValueType {
    VALUE_INT,
    VALUE_BOOL,
    VALUE_STRING,
    VALUE_TUPLE,
    VALUE_CLOSURE,
    VALUE_DUMMY,
    VALUE_NIL,
    VALUE_BUILTIN
} ValueType;

typedef struct Env Env;

typedef struct Value {
    ValueType type;
    long integer_value;
    int boolean_value;
    char *string_value;
    struct Value **tuple_items;
    size_t tuple_size;
    char **params;
    size_t param_count;
    ASTNode *body;
    Env *closure_env;
    char *builtin_name;
} Value;

struct Env {
    char *name;
    Value *value;
    Env *next;
};

static Value *eval_node(ASTNode *node, Env *env);
static Value *apply_value(Value *function_value, Value *argument_value);
static Value *apply_builtin(const char *name, Value *argument_value);
static Value *eval_definition(ASTNode *definition, Env *env, Env **out_env);
static Value *eval_simple_definition(ASTNode *definition, Env *env, Env **out_env);
static Value *eval_rec_definition(ASTNode *definition, Env *env, Env **out_env);
static Value *eval_and_definition(ASTNode *definition, Env *env, Env **out_env);
static Env *env_extend(Env *env, const char *name, Value *value);
static Value *env_lookup(Env *env, const char *name);
static char **collect_params(ASTNode *params_node, size_t *count);
static void collect_param_names(ASTNode *node, char ***names, size_t *count);
static Value *make_value(ValueType type);
static Value *make_int_value(long number);
static Value *make_bool_value(int boolean_value);
static Value *make_string_value(const char *text);
static Value *make_tuple_value(size_t size);
static Value *make_closure_value(ASTNode *body, Env *env, char **params, size_t count);
static Value *make_builtin_value(const char *name);
static Value *make_dummy_value(void);
static Value *make_nil_value(void);
static Value *copy_value(Value *value);
static int value_to_bool(Value *value);
static long value_to_int(Value *value);
static void print_value(Value *value);
static void print_tuple(Value *value);
static int is_integer_text(const char *text);
static char *decode_string_literal(const char *text);
static char *duplicate_text(const char *text);
static void free_value(Value *value);
static char *join_strings(const char *left, const char *right);
static Value *apply_lambda(Value *closure_value, Value *argument_value);
static Value *make_tuple_from_children(ASTNode *node, Env *env);
static Value *bind_pattern(ASTNode *pattern, Value *value, Env *env, Env **out_env);
static Value *bind_name(const char *name, Value *value, Env *env, Env **out_env);
static int is_identifier_node(ASTNode *node);

int cse_evaluate(ASTNode *root)
{
    Value *result = NULL;

    if (root == NULL) {
        return 1;
    }

    result = eval_node(root, NULL);
    if (result == NULL) {
        return 1;
    }

    print_value(result);
    putchar('\n');
    return 0;
}

static Value *eval_node(ASTNode *node, Env *env)
{
    ASTNode *child = NULL;

    if (node == NULL) {
        return make_dummy_value();
    }

    if (node->first_child == NULL) {
        if (strcmp(node->label, "true") == 0) {
            return make_bool_value(1);
        }
        if (strcmp(node->label, "false") == 0) {
            return make_bool_value(0);
        }
        if (strcmp(node->label, "nil") == 0) {
            return make_nil_value();
        }
        if (strcmp(node->label, "dummy") == 0 || strcmp(node->label, "()") == 0) {
            return make_dummy_value();
        }
        if (is_integer_text(node->label)) {
            return make_int_value(strtol(node->label, NULL, 10));
        }
        if (node->label[0] == '\'' && node->label[strlen(node->label) - 1] == '\'') {
            return make_string_value(node->label);
        }

        {
            Value *binding = env_lookup(env, node->label);
            if (binding != NULL) {
                return copy_value(binding);
            }

            if (strcmp(node->label, "Print") == 0 || strcmp(node->label, "Order") == 0) {
                return make_builtin_value(node->label);
            }
        }

        fprintf(stderr, "Error: unbound identifier '%s'\n", node->label);
        return NULL;
    }

    if (strcmp(node->label, "gamma") == 0) {
        Value *function_value = eval_node(node->first_child, env);
        Value *argument_value = NULL;
        Value *result = NULL;

        if (function_value == NULL) {
            return NULL;
        }

        argument_value = eval_node(node->first_child->next_sibling, env);
        if (argument_value == NULL) {
            free_value(function_value);
            return NULL;
        }

        result = apply_value(function_value, argument_value);
        free_value(function_value);
        free_value(argument_value);
        return result;
    }

    if (strcmp(node->label, "lambda") == 0) {
        size_t count = 0;
        char **params = collect_params(node->first_child, &count);
        if (params == NULL && count != 0) {
            return NULL;
        }
        return make_closure_value(node->first_child->next_sibling, env, params, count);
    }

    if (strcmp(node->label, "let") == 0) {
        Env *extended = NULL;
        Value *ignored = eval_definition(node->first_child, env, &extended);
        Value *body = NULL;

        if (ignored == NULL) {
            return NULL;
        }

        body = eval_node(node->first_child->next_sibling, extended);
        return body;
    }

    if (strcmp(node->label, "rec") == 0) {
        Env *extended = NULL;
        Value *result = eval_rec_definition(node->first_child, env, &extended);
        if (result != NULL && result->type == VALUE_BUILTIN) {
            return result;
        }
        return result;
    }

    if (strcmp(node->label, "and") == 0) {
        Env *extended = NULL;
        Value *ignored = eval_and_definition(node, env, &extended);
        (void)ignored;
        return make_dummy_value();
    }

    if (strcmp(node->label, "where") == 0) {
        Value *defs = NULL;
        Value *body = NULL;
        Env *extended = NULL;

        defs = eval_definition(node->first_child->next_sibling, env, &extended);
        if (defs == NULL) {
            return NULL;
        }
        body = eval_node(node->first_child, extended);
        return body;
    }

    if (strcmp(node->label, "tau") == 0) {
        return make_tuple_from_children(node, env);
    }

    if (strcmp(node->label, "aug") == 0) {
        Value *left = eval_node(node->first_child, env);
        Value *right = eval_node(node->first_child->next_sibling, env);
        Value *tuple = NULL;
        size_t index = 0;

        if (left == NULL || right == NULL) {
            free_value(left);
            free_value(right);
            return NULL;
        }

        if (left->type == VALUE_TUPLE) {
            tuple = make_tuple_value(left->tuple_size + 1);
            for (index = 0; index < left->tuple_size; index++) {
                tuple->tuple_items[index] = copy_value(left->tuple_items[index]);
            }
            tuple->tuple_items[left->tuple_size] = copy_value(right);
        } else if (left->type == VALUE_NIL) {
            tuple = make_tuple_value(1);
            tuple->tuple_items[0] = copy_value(right);
        } else {
            tuple = make_tuple_value(2);
            tuple->tuple_items[0] = copy_value(left);
            tuple->tuple_items[1] = copy_value(right);
        }

        free_value(left);
        free_value(right);
        return tuple;
    }

    if (strcmp(node->label, "->") == 0) {
        Value *condition = eval_node(node->first_child, env);
        int truth = 0;

        if (condition == NULL) {
            return NULL;
        }
        truth = value_to_bool(condition);
        free_value(condition);
        if (truth) {
            return eval_node(node->first_child->next_sibling, env);
        }
        return eval_node(node->first_child->next_sibling->next_sibling, env);
    }

    if (strcmp(node->label, "not") == 0) {
        Value *child_value = eval_node(node->first_child, env);
        Value *result = NULL;
        if (child_value == NULL) {
            return NULL;
        }
        result = make_bool_value(!value_to_bool(child_value));
        free_value(child_value);
        return result;
    }

    if (strcmp(node->label, "neg") == 0) {
        Value *child_value = eval_node(node->first_child, env);
        Value *result = NULL;
        if (child_value == NULL) {
            return NULL;
        }
        result = make_int_value(-value_to_int(child_value));
        free_value(child_value);
        return result;
    }

    if (strcmp(node->label, "uplus") == 0) {
        return eval_node(node->first_child, env);
    }

    if (strcmp(node->label, "+") == 0 || strcmp(node->label, "-") == 0 ||
        strcmp(node->label, "*") == 0 || strcmp(node->label, "/") == 0 ||
        strcmp(node->label, "**") == 0 || strcmp(node->label, "gr") == 0 ||
        strcmp(node->label, "ge") == 0 || strcmp(node->label, "ls") == 0 ||
        strcmp(node->label, "le") == 0 || strcmp(node->label, "eq") == 0 ||
        strcmp(node->label, "ne") == 0 || strcmp(node->label, "or") == 0 ||
        strcmp(node->label, "&") == 0) {
        Value *left = eval_node(node->first_child, env);
        Value *right = eval_node(node->first_child->next_sibling, env);
        Value *result = NULL;
        long lhs = 0;
        long rhs = 0;

        if (left == NULL || right == NULL) {
            free_value(left);
            free_value(right);
            return NULL;
        }

        if (strcmp(node->label, "+") == 0) {
            result = make_int_value(value_to_int(left) + value_to_int(right));
        } else if (strcmp(node->label, "-") == 0) {
            result = make_int_value(value_to_int(left) - value_to_int(right));
        } else if (strcmp(node->label, "*") == 0) {
            result = make_int_value(value_to_int(left) * value_to_int(right));
        } else if (strcmp(node->label, "/") == 0) {
            rhs = value_to_int(right);
            result = make_int_value(rhs == 0 ? 0 : value_to_int(left) / rhs);
        } else if (strcmp(node->label, "**") == 0) {
            long power = value_to_int(right);
            long base = value_to_int(left);
            long value = 1;
            while (power-- > 0) {
                value *= base;
            }
            result = make_int_value(value);
        } else if (strcmp(node->label, "gr") == 0) {
            result = make_bool_value(value_to_int(left) > value_to_int(right));
        } else if (strcmp(node->label, "ge") == 0) {
            result = make_bool_value(value_to_int(left) >= value_to_int(right));
        } else if (strcmp(node->label, "ls") == 0) {
            result = make_bool_value(value_to_int(left) < value_to_int(right));
        } else if (strcmp(node->label, "le") == 0) {
            result = make_bool_value(value_to_int(left) <= value_to_int(right));
        } else if (strcmp(node->label, "eq") == 0) {
            result = make_bool_value(value_to_int(left) == value_to_int(right));
        } else if (strcmp(node->label, "ne") == 0) {
            result = make_bool_value(value_to_int(left) != value_to_int(right));
        } else if (strcmp(node->label, "or") == 0) {
            result = make_bool_value(value_to_bool(left) || value_to_bool(right));
        } else if (strcmp(node->label, "&") == 0) {
            result = make_bool_value(value_to_bool(left) && value_to_bool(right));
        }

        free_value(left);
        free_value(right);
        return result;
    }

    if (strcmp(node->label, "@")==0) {
        Value *function_value = eval_node(node->first_child->next_sibling, env);
        Value *argument_value = eval_node(node->first_child, env);
        Value *result = NULL;

        if (function_value == NULL || argument_value == NULL) {
            free_value(function_value);
            free_value(argument_value);
            return NULL;
        }

        result = apply_value(function_value, argument_value);
        free_value(function_value);
        free_value(argument_value);
        return result;
    }

    if (strcmp(node->label, "=") == 0) {
        Env *extended = NULL;
        Value *ignored = eval_simple_definition(node, env, &extended);
        if (ignored == NULL) {
            return NULL;
        }
        return make_dummy_value();
    }

    {
        Value *result = NULL;
        ASTNode *child_node = NULL;
        for (child_node = node->first_child; child_node != NULL; child_node = child_node->next_sibling) {
            result = eval_node(child_node, env);
            if (result == NULL) {
                return NULL;
            }
        }
        return result != NULL ? result : make_dummy_value();
    }
}

static Value *apply_value(Value *function_value, Value *argument_value)
{
    if (function_value == NULL) {
        return NULL;
    }

    if (function_value->type == VALUE_BUILTIN) {
        return apply_builtin(function_value->builtin_name, argument_value);
    }

    if (function_value->type == VALUE_CLOSURE) {
        return apply_lambda(function_value, argument_value);
    }

    fprintf(stderr, "Error: attempted to apply a non-function value\n");
    return NULL;
}

static Value *apply_builtin(const char *name, Value *argument_value)
{
    if (name == NULL) {
        return NULL;
    }

    if (strcmp(name, "Print") == 0) {
        return copy_value(argument_value);
    }

    if (strcmp(name, "Order") == 0) {
        if (argument_value != NULL && argument_value->type == VALUE_TUPLE) {
            return make_int_value((long)argument_value->tuple_size);
        }
        return make_int_value(0);
    }

    fprintf(stderr, "Error: unknown builtin '%s'\n", name);
    return NULL;
}

static Value *apply_lambda(Value *closure_value, Value *argument_value)
{
    Env *extended = NULL;
    Value *result = NULL;

    if (closure_value == NULL || closure_value->type != VALUE_CLOSURE) {
        return NULL;
    }

    if (closure_value->param_count == 0) {
        return eval_node(closure_value->body, closure_value->closure_env);
    }

    extended = env_extend(closure_value->closure_env, closure_value->params[0], argument_value);
    if (closure_value->param_count == 1) {
        result = eval_node(closure_value->body, extended);
        return result;
    }

    result = make_closure_value(closure_value->body, extended, closure_value->params + 1, closure_value->param_count - 1);
    return result;
}

static Value *eval_definition(ASTNode *definition, Env *env, Env **out_env)
{
    if (definition == NULL) {
        if (out_env != NULL) {
            *out_env = env;
        }
        return make_dummy_value();
    }

    if (strcmp(definition->label, "=") == 0) {
        return eval_simple_definition(definition, env, out_env);
    }

    if (strcmp(definition->label, "rec") == 0) {
        return eval_rec_definition(definition, env, out_env);
    }

    if (strcmp(definition->label, "and") == 0) {
        return eval_and_definition(definition, env, out_env);
    }

    if (definition->first_child != NULL) {
        return eval_definition(definition->first_child, env, out_env);
    }

    if (out_env != NULL) {
        *out_env = env;
    }
    return make_dummy_value();
}

static Value *eval_simple_definition(ASTNode *definition, Env *env, Env **out_env)
{
    ASTNode *pattern = NULL;
    ASTNode *expression = NULL;
    Value *value = NULL;
    Env *extended = env;

    if (definition == NULL || definition->first_child == NULL || definition->first_child->next_sibling == NULL) {
        if (out_env != NULL) {
            *out_env = env;
        }
        return make_dummy_value();
    }

    pattern = definition->first_child;
    expression = definition->first_child->next_sibling;
    value = eval_node(expression, env);
    if (value == NULL) {
        return NULL;
    }

    if (!bind_pattern(pattern, value, env, &extended)) {
        free_value(value);
        return NULL;
    }

    if (out_env != NULL) {
        *out_env = extended;
    }

    return value;
}

static Value *eval_rec_definition(ASTNode *definition, Env *env, Env **out_env)
{
    ASTNode *child = NULL;
    ASTNode *pattern = NULL;
    ASTNode *expression = NULL;
    Value *value = NULL;
    Env *extended = env;

    if (definition == NULL || definition->first_child == NULL) {
        if (out_env != NULL) {
            *out_env = env;
        }
        return make_dummy_value();
    }

    child = definition->first_child;
    if (strcmp(child->label, "=") == 0) {
        pattern = child->first_child;
        expression = child->first_child != NULL ? child->first_child->next_sibling : NULL;
    } else {
        pattern = child;
        expression = NULL;
    }

    if (pattern == NULL || expression == NULL) {
        if (out_env != NULL) {
            *out_env = env;
        }
        return make_dummy_value();
    }

    if (is_identifier_node(pattern) && expression->first_child != NULL && strcmp(expression->label, "lambda") == 0) {
        size_t count = 0;
        char **params = collect_params(expression->first_child, &count);
        Env *recursive_env = env_extend(env, pattern->label, NULL);
        Value *closure = NULL;

        if (params == NULL && count != 0) {
            return NULL;
        }

        closure = make_closure_value(expression->first_child->next_sibling, recursive_env, params, count);
        recursive_env->value = closure;
        if (out_env != NULL) {
            *out_env = recursive_env;
        }
        return closure;
    }

    value = eval_node(expression, env);
    if (value == NULL) {
        return NULL;
    }

    if (!bind_pattern(pattern, value, env, &extended)) {
        free_value(value);
        return NULL;
    }

    if (out_env != NULL) {
        *out_env = extended;
    }

    return value;
}

static Value *eval_and_definition(ASTNode *definition, Env *env, Env **out_env)
{
    ASTNode *child = NULL;
    Env *extended = env;
    Value *result = make_dummy_value();

    if (definition == NULL) {
        if (out_env != NULL) {
            *out_env = env;
        }
        return result;
    }

    for (child = definition->first_child; child != NULL; child = child->next_sibling) {
        if (strcmp(child->label, "=") == 0) {
            Value *value = NULL;
            ASTNode *pattern = child->first_child;
            ASTNode *expression = pattern != NULL ? pattern->next_sibling : NULL;
            if (pattern == NULL || expression == NULL) {
                continue;
            }
            value = eval_node(expression, env);
            if (value == NULL) {
                return NULL;
            }
            if (!bind_pattern(pattern, value, env, &extended)) {
                free_value(value);
                return NULL;
            }
        } else {
            Env *child_env = env;
            Value *value = eval_definition(child, env, &child_env);
            if (value == NULL) {
                return NULL;
            }
            extended = child_env;
            (void)value;
        }
    }

    if (out_env != NULL) {
        *out_env = extended;
    }

    return result;
}

static Env *env_extend(Env *env, const char *name, Value *value)
{
    Env *entry = (Env *)malloc(sizeof(Env));
    if (entry == NULL) {
        return env;
    }

    entry->name = duplicate_text(name);
    entry->value = value;
    entry->next = env;
    return entry;
}

static Value *env_lookup(Env *env, const char *name)
{
    Env *current = NULL;

    for (current = env; current != NULL; current = current->next) {
        if (current->name != NULL && name != NULL && strcmp(current->name, name) == 0) {
            return current->value;
        }
    }

    return NULL;
}

static char **collect_params(ASTNode *params_node, size_t *count)
{
    char **names = NULL;
    size_t capacity = 0;

    if (count != NULL) {
        *count = 0;
    }

    if (params_node == NULL) {
        return NULL;
    }

    collect_param_names(params_node, &names, &capacity);
    if (count != NULL) {
        *count = capacity;
    }
    return names;
}

static void collect_param_names(ASTNode *node, char ***names, size_t *count)
{
    ASTNode *child = NULL;

    if (node == NULL || names == NULL || count == NULL) {
        return;
    }

    if (is_identifier_node(node)) {
        char **items = (char **)realloc(*names, (*count + 1) * sizeof(char *));
        if (items == NULL) {
            return;
        }
        *names = items;
        (*names)[*count] = duplicate_text(node->label);
        (*count)++;
        return;
    }

    if (strcmp(node->label, "Vl") == 0 || strcmp(node->label, "params") == 0 || strcmp(node->label, "tau") == 0) {
        for (child = node->first_child; child != NULL; child = child->next_sibling) {
            collect_param_names(child, names, count);
        }
        return;
    }

    if (strcmp(node->label, "()") == 0) {
        return;
    }

    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        collect_param_names(child, names, count);
    }
}

static Value *make_value(ValueType type)
{
    Value *value = (Value *)calloc(1, sizeof(Value));
    if (value == NULL) {
        return NULL;
    }
    value->type = type;
    return value;
}

static Value *make_int_value(long number)
{
    Value *value = make_value(VALUE_INT);
    if (value != NULL) {
        value->integer_value = number;
    }
    return value;
}

static Value *make_bool_value(int boolean_value)
{
    Value *value = make_value(VALUE_BOOL);
    if (value != NULL) {
        value->boolean_value = boolean_value ? 1 : 0;
    }
    return value;
}

static Value *make_string_value(const char *text)
{
    Value *value = make_value(VALUE_STRING);
    if (value != NULL) {
        value->string_value = decode_string_literal(text);
    }
    return value;
}

static Value *make_tuple_value(size_t size)
{
    Value *value = make_value(VALUE_TUPLE);
    if (value != NULL) {
        value->tuple_items = (Value **)calloc(size == 0 ? 1 : size, sizeof(Value *));
        value->tuple_size = size;
    }
    return value;
}

static Value *make_closure_value(ASTNode *body, Env *env, char **params, size_t count)
{
    Value *value = make_value(VALUE_CLOSURE);
    if (value != NULL) {
        value->body = body;
        value->closure_env = env;
        value->params = params;
        value->param_count = count;
    }
    return value;
}

static Value *make_builtin_value(const char *name)
{
    Value *value = make_value(VALUE_BUILTIN);
    if (value != NULL) {
        value->builtin_name = duplicate_text(name);
    }
    return value;
}

static Value *make_dummy_value(void)
{
    return make_value(VALUE_DUMMY);
}

static Value *make_nil_value(void)
{
    return make_value(VALUE_NIL);
}

static Value *copy_value(Value *value)
{
    size_t index = 0;

    if (value == NULL) {
        return NULL;
    }

    switch (value->type) {
    case VALUE_INT:
        return make_int_value(value->integer_value);
    case VALUE_BOOL:
        return make_bool_value(value->boolean_value);
    case VALUE_STRING:
        return make_string_value(value->string_value != NULL ? value->string_value : "");
    case VALUE_TUPLE: {
        Value *tuple = make_tuple_value(value->tuple_size);
        if (tuple == NULL) {
            return NULL;
        }
        for (index = 0; index < value->tuple_size; index++) {
            tuple->tuple_items[index] = copy_value(value->tuple_items[index]);
        }
        return tuple;
    }
    case VALUE_CLOSURE: {
        char **params = NULL;
        if (value->param_count > 0) {
            params = (char **)calloc(value->param_count, sizeof(char *));
            if (params == NULL) {
                return NULL;
            }
            for (index = 0; index < value->param_count; index++) {
                params[index] = duplicate_text(value->params[index]);
            }
        }
        return make_closure_value(value->body, value->closure_env, params, value->param_count);
    }
    case VALUE_DUMMY:
        return make_dummy_value();
    case VALUE_NIL:
        return make_nil_value();
    case VALUE_BUILTIN:
        return make_builtin_value(value->builtin_name);
    default:
        return NULL;
    }
}

static int value_to_bool(Value *value)
{
    if (value == NULL) {
        return 0;
    }

    switch (value->type) {
    case VALUE_BOOL:
        return value->boolean_value;
    case VALUE_INT:
        return value->integer_value != 0;
    case VALUE_NIL:
        return 0;
    case VALUE_DUMMY:
        return 0;
    default:
        return 1;
    }
}

static long value_to_int(Value *value)
{
    if (value == NULL) {
        return 0;
    }

    if (value->type == VALUE_INT) {
        return value->integer_value;
    }

    if (value->type == VALUE_BOOL) {
        return value->boolean_value ? 1 : 0;
    }

    return 0;
}

static void print_value(Value *value)
{
    if (value == NULL) {
        return;
    }

    switch (value->type) {
    case VALUE_INT:
        printf("%ld", value->integer_value);
        break;
    case VALUE_BOOL:
        printf(value->boolean_value ? "true" : "false");
        break;
    case VALUE_STRING:
        printf("%s", value->string_value != NULL ? value->string_value : "");
        break;
    case VALUE_TUPLE:
        print_tuple(value);
        break;
    case VALUE_DUMMY:
        printf("dummy");
        break;
    case VALUE_NIL:
        printf("nil");
        break;
    case VALUE_CLOSURE:
        printf("<function>");
        break;
    case VALUE_BUILTIN:
        printf("<builtin:%s>", value->builtin_name != NULL ? value->builtin_name : "?");
        break;
    }
}

static void print_tuple(Value *value)
{
    size_t index = 0;

    if (value == NULL || value->type != VALUE_TUPLE) {
        return;
    }

    putchar('(');
    for (index = 0; index < value->tuple_size; index++) {
        if (index > 0) {
            printf(", ");
        }
        print_value(value->tuple_items[index]);
    }
    putchar(')');
}

static int is_integer_text(const char *text)
{
    size_t index = 0;

    if (text == NULL || *text == '\0') {
        return 0;
    }

    if (text[0] == '-' || text[0] == '+') {
        index = 1;
    }

    if (text[index] == '\0') {
        return 0;
    }

    for (; text[index] != '\0'; index++) {
        if (!isdigit((unsigned char)text[index])) {
            return 0;
        }
    }

    return 1;
}

static char *decode_string_literal(const char *text)
{
    size_t length = 0;
    size_t index = 0;
    size_t out = 0;
    char *result = NULL;

    if (text == NULL) {
        return duplicate_text("");
    }

    length = strlen(text);
    if (length < 2 || text[0] != '\'' || text[length - 1] != '\'') {
        return duplicate_text(text);
    }

    result = (char *)malloc(length + 1);
    if (result == NULL) {
        return NULL;
    }

    for (index = 1; index + 1 < length; index++) {
        if (text[index] == '\\' && index + 1 < length - 1) {
            index++;
            switch (text[index]) {
            case 'n':
                result[out++] = '\n';
                break;
            case 't':
                result[out++] = '\t';
                break;
            case '\\':
                result[out++] = '\\';
                break;
            case '\'':
                result[out++] = '\'';
                break;
            default:
                result[out++] = text[index];
                break;
            }
        } else {
            result[out++] = text[index];
        }
    }

    result[out] = '\0';
    return result;
}

static char *duplicate_text(const char *text)
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

static void free_value(Value *value)
{
    size_t index = 0;

    if (value == NULL) {
        return;
    }

    if (value->type == VALUE_TUPLE && value->tuple_items != NULL) {
        for (index = 0; index < value->tuple_size; index++) {
            free_value(value->tuple_items[index]);
        }
        free(value->tuple_items);
    }

    if (value->type == VALUE_CLOSURE && value->params != NULL) {
        for (index = 0; index < value->param_count; index++) {
            free(value->params[index]);
        }
        free(value->params);
    }

    free(value->string_value);
    free(value->builtin_name);
    free(value);
}

static Value *make_tuple_from_children(ASTNode *node, Env *env)
{
    ASTNode *child = NULL;
    Value *tuple = NULL;
    size_t count = 0;
    size_t index = 0;

    if (node == NULL) {
        return make_tuple_value(0);
    }

    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        count++;
    }

    tuple = make_tuple_value(count);
    if (tuple == NULL) {
        return NULL;
    }

    for (child = node->first_child; child != NULL; child = child->next_sibling) {
        tuple->tuple_items[index++] = eval_node(child, env);
    }

    return tuple;
}

static Value *bind_pattern(ASTNode *pattern, Value *value, Env *env, Env **out_env)
{
    ASTNode *child = NULL;
    Env *current = env;

    if (pattern == NULL) {
        if (out_env != NULL) {
            *out_env = env;
        }
        return value;
    }

    if (is_identifier_node(pattern)) {
        current = env_extend(env, pattern->label, value);
        if (out_env != NULL) {
            *out_env = current;
        }
        return value;
    }

    if (strcmp(pattern->label, "Vl") == 0 || strcmp(pattern->label, "params") == 0) {
        for (child = pattern->first_child; child != NULL; child = child->next_sibling) {
            if (!is_identifier_node(child)) {
                continue;
            }
            current = env_extend(current, child->label, value != NULL && value->type == VALUE_TUPLE && child->next_sibling != NULL
                                                  ? value->tuple_items[0]
                                                  : value);
        }
        if (out_env != NULL) {
            *out_env = current;
        }
        return value;
    }

    if (strcmp(pattern->label, "tau") == 0) {
        if (value == NULL || value->type != VALUE_TUPLE) {
            fprintf(stderr, "Error: tuple pattern matched with non-tuple value\n");
            return NULL;
        }

        child = pattern->first_child;
        for (size_t index = 0; child != NULL && index < value->tuple_size; index++, child = child->next_sibling) {
            if (!bind_pattern(child, value->tuple_items[index], current, &current)) {
                return NULL;
            }
        }

        if (out_env != NULL) {
            *out_env = current;
        }
        return value;
    }

    if (strcmp(pattern->label, "()")==0) {
        if (out_env != NULL) {
            *out_env = current;
        }
        return value;
    }

    if (pattern->first_child != NULL) {
        for (child = pattern->first_child; child != NULL; child = child->next_sibling) {
            if (!bind_pattern(child, value, current, &current)) {
                return NULL;
            }
        }
    }

    if (out_env != NULL) {
        *out_env = current;
    }

    return value;
}

static Value *bind_name(const char *name, Value *value, Env *env, Env **out_env)
{
    Env *extended = env_extend(env, name, value);
    if (out_env != NULL) {
        *out_env = extended;
    }
    return value;
}

static int is_identifier_node(ASTNode *node)
{
    if (node == NULL || node->label == NULL) {
        return 0;
    }

    if (strcmp(node->label, "true") == 0 || strcmp(node->label, "false") == 0 ||
        strcmp(node->label, "nil") == 0 || strcmp(node->label, "dummy") == 0) {
        return 0;
    }

    if (node->first_child != NULL) {
        return 0;
    }

    return !is_integer_text(node->label) && node->label[0] != '\'';
}
