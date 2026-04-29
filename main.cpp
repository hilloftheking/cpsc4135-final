#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "gc.hpp"
#include "sexp.hpp"

using Scope = std::unordered_map<std::string, SExpression *>;

std::vector<Scope> environment;

bool should_quit = false;

bool is_whitespace(char c) { return c == ' ' || c == '\n' || c == '\t'; }

// Returns true if c marks the end of a symbol
bool is_end_of_symbol(char c) {
  return is_whitespace(c) || c == '(' || c == ')';
}

bool is_numeric(char c) { return c >= '0' && c <= '9'; }

int parse_num(const std::string &s, size_t &i) {
  int num = 0;

  while (i < s.length()) {
    char c = s[i];
    if (!is_numeric(c)) {
      // TODO: check for invalid character within number
      break;
    }

    num *= 10;
    num += c - '0';

    i++;
  }

  // Now i is at the point right after the number

  return num;
}

std::string parse_sym(const std::string &s, size_t &i) {
  std::string sym;

  while (i < s.length()) {
    char c = s[i];
    if (is_end_of_symbol(c)) {
      break;
    }

    sym += c;
    i++;
  }

  // Now i is at the point right after the symbol

  return sym;
}

SExpression *eval_sexp(SExpression *expression) {
  if (expression->is_cons()) {
    // This expression is the start of a list

    // Eval the first element of the list to get the function expression
    SExpression *func = eval_sexp(expression->cons.car);

    SExpression *result;

    if (func->is_native_function()) {
      SExpression *current = expression->cons.cdr;
      while (current->is_cons()) {
        // Each car can be replaced with an evaluated version :)
        SExpression *new_arg = eval_sexp(current->cons.car);
        current->cons.car = new_arg;

        current = current->cons.cdr;
      }

      result = func->native_procedure(expression->cons.cdr);
    } else if (func->is_special_operator()) {
      result = func->native_procedure(expression->cons.cdr);
    } else {
      std::cout << "ERROR: Invalid function." << std::endl;
      result = make_nil();
    }

    return result;
  } else if (expression->is_symbol()) {
    const std::string &sym = *expression->atom.symbol;

    // Go backwards through the environment in order to prioritize the most
    // local scope
    for (auto scope_it = environment.rbegin(); scope_it != environment.rend();
         scope_it++) {
      Scope &scope = *scope_it;

      auto sexpr_it = scope.find(sym);
      if (sexpr_it != scope.end()) {
        return sexpr_it->second;
      }
    }

    // Variable was not found
    std::cout << "ERROR: Variable " << sym << " does not exist." << std::endl;
    return make_nil();
  } else {
    return expression;
  }
}

SExpression *quote(SExpression *expression) {
  if (!expression->is_cons()) {
    std::cout << "ERROR: quote expects one argument." << std::endl;
    return make_nil();
  }

  return expression->cons.car;
}

SExpression *define(SExpression *expression) {
  do {
    if (!expression->is_cons())
      break;

    SExpression *symbol = expression->cons.car;
    if (!symbol->is_symbol())
      break;

    SExpression *second = expression->cons.cdr;
    if (!second->is_cons())
      break;

    SExpression *value_exp = second->cons.car;
    SExpression *value = eval_sexp(value_exp);
    // Use the global scope when using define
    environment.front()[*symbol->atom.symbol] = value;

    return value;
  } while (false);

  std::cout << "ERROR: define expects two arguments, where the first is a "
               "symbol, and the second is the value expression."
            << std::endl;
  return make_nil();
}

SExpression *add(SExpression *expression) {
  int result = 0;
  while (expression->is_cons()) {
    SExpression *car = expression->cons.car;
    if (car->is_number()) {
      result += car->atom.number;
    }

    expression = expression->cons.cdr;
  }

  return make_number(result);
}

SExpression *subtract(SExpression *expression) {
  int result = 0;
  bool is_first = true;
  while (expression->is_cons()) {
    SExpression *car = expression->cons.car;
    if (car->is_number()) {
      if (is_first) {
        is_first = false;
        result = car->atom.number;
      } else {
        result -= car->atom.number;
      }
    }

    expression = expression->cons.cdr;
  }

  return make_number(result);
}

SExpression *multiply(SExpression *expression) {
  int result = 1;
  while (expression->is_cons()) {
    SExpression *car = expression->cons.car;
    if (car->is_number()) {
      result *= car->atom.number;
    }

    expression = expression->cons.cdr;
  }

  return make_number(result);
}

SExpression *divide(SExpression *expression) {
  int result = 0;
  bool is_first = true;
  while (expression->is_cons()) {
    SExpression *car = expression->cons.car;
    if (car->is_number()) {
      if (is_first) {
        is_first = false;
        result = car->atom.number;
      } else {
        result /= car->atom.number;
      }
    }

    expression = expression->cons.cdr;
  }

  return make_number(result);
}

SExpression *list(SExpression *expression) {
  if (expression->is_nil()) {
    return make_nil();
  } else {
    return expression;
  }
}

SExpression *quit(SExpression *expression) {
  should_quit = true;
  return make_nil();
}

SExpression *eval(SExpression *expression) {
  if (!expression->is_cons() || !expression->cons.cdr->is_nil()) {
    std::cout << "ERROR: eval expects one argument." << std::endl;
    return make_nil();
  }

  return eval_sexp(expression->cons.car);
}

SExpression *execute_string(const std::string &s) {
  // (1 2 3) -> (1 . (2 . (3 . nil)))
  // ( - Create cons cell
  // 1 - Set car of current cell to 1
  //   - Whitespace: Finish current int
  // 2 - Set cdr of current cell to new cons cell, where its car is set to 2
  // And so on

  // () -> nil

  SExpression *current = make_nil();

  // Whenever a list is closed with ')', the previous cons before the start of
  // the list must be the new current expresssion
  std::stack<SExpression *> prev_cons;

  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];

    if (is_whitespace(c))
      continue;

    if (c == ')') {
      if (prev_cons.empty()) {
        std::cout << "ERROR: Unmatched right parenthesis." << std::endl;
        return make_nil();
      }

      if (current->cons.car == nullptr) {
        // If the car of this cons cell is nullptr, that means that this is an
        // empty list -> ().
        // So this cons cell expression should be turned into a nil expression.
        current->type = SExpression::TYPE_ATOM;
        current->atom.type = Atom::TYPE_NIL;
      } else {
        // A non empty list just got finished. The cdr of the current cons cell
        // should become nil instead of nullptr.
        current->cons.cdr = make_nil();
      }

      // The current chain of cons cells has to end. This means that the
      // previous cons before the start of the current cons chain is now the
      // current expression.
      current = prev_cons.top();
      prev_cons.pop();

      if (prev_cons.empty()) {
        // The root list just got finished, so it should be evaluated now.
        current = eval_sexp(current);
      }

      continue;
    }

    // This is the new expression that will be parsed and inserted into the
    // current expression
    SExpression *new_expression = nullptr;

    if (c == '(') {
      // Start of list
      // This created cons cell is special because it stores nullptr instead of
      // nil expressions.
      new_expression = make_cons(nullptr, nullptr);
    } else if (is_numeric(c)) {
      // Number
      int num = parse_num(s, i);
      new_expression = make_number(num);

      // After parsing a num, i is set to immediately after the number within
      // the string. So it must be decremented since it will be incremented
      // later.
      i--;
    } else {
      // Symbol
      std::string sym = parse_sym(s, i);
      new_expression = make_symbol(sym);

      // Same as above, i must be decremented.
      i--;
    }

    // Now insert new expression into the current expression

    if (!current->is_cons() || current->cons.cdr != nullptr) {
      // Either not inside of a cons cell, or the cons cell is full.
      // So the current expression should just be overwritten.
      current = new_expression;

      if (!current->is_cons()) {
        // A list is not being created, so whatever expression got made should
        // just be evaluated right now.
        current = eval_sexp(current);
      }
    } else {
      // Current expression is a cons cell that the new expression must be
      // inserted into.
      if (current->cons.car == nullptr) {
        current->cons.car = new_expression;
      } else {
        // The cdr of the current cons cell is nullptr, so it will now be a cons
        // with the new expression as its car.
        current->cons.cdr = make_cons(new_expression, nullptr);
        current = current->cons.cdr;
      }
    }

    // If the new expression was a cons cell, it should become the new current
    // expression
    if (new_expression->is_cons()) {
      prev_cons.push(current);
      current = new_expression;
    }
  }

  if (!prev_cons.empty()) {
    // There is at least one unclosed list

    // Keep popping from the stack until back at the original expression
    while (!prev_cons.empty()) {
      current = prev_cons.top();
      prev_cons.pop();
    }

    std::cout << "ERROR: Unmatched left parenthesis." << std::endl;
    return make_nil();
  }

  return current;
}

int main() {
  environment.push_back({});
  Scope &globals = environment.back();

  globals["nil"] = make_nil();

  globals["quote"] = make_special_operator(quote);
  globals["define"] = make_special_operator(define);
  globals["+"] = make_native_function(add);
  globals["-"] = make_native_function(subtract);
  globals["*"] = make_native_function(multiply);
  globals["/"] = make_native_function(divide);
  globals["list"] = make_native_function(list);
  globals["quit"] = make_native_function(quit);
  globals["eval"] = make_native_function(eval);

  std::string input;

  while (!should_quit) {
    std::cout << "> ";
    std::getline(std::cin, input);

    SExpression *result = execute_string(input);
    std::cout << result->as_string() << std::endl;

    gc_collect(globals);
  }
}
