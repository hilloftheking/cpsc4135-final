#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct SExpression;

using Scope = std::unordered_map<std::string, SExpression *>;

extern std::vector<Scope> environment;

bool should_quit();

SExpression *eval_sexp(SExpression *sexp);

void create_globals();
