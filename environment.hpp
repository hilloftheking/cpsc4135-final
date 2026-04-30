#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct SExpression;

using Scope = std::unordered_map<std::string, SExpression *>;

extern std::vector<Scope> environment;

bool should_quit();

// Evaluates an expression and returns the result.
SExpression *eval_sexp(SExpression *sexp);

// Evaluates multiple expressions sequentially and returns the final result.
SExpression *eval_sexps(std::vector<SExpression *> expressions);

// Registers the global variables and functions in the environment.
void create_globals();
