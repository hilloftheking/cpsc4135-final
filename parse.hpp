#pragma once

#include <string>
#include <vector>

struct SExpression;

// Parses a string into a vector of expressions that should be evaluated
std::vector<SExpression *> parse_string(const std::string &s);
