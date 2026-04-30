#include <fstream>
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

SExpression *progn(SExpression *expression);

SExpression *eval_sexp(SExpression *expression) {
  if (expression->is_cons()) {
    // This expression is the start of a list

    // Eval the first element of the list to get the function expression
    SExpression *func = eval_sexp(expression->cons.car);

    SExpression *result;

    if (func->is_native_function()) {
      // Create a new list of evaluated arguments
      SExpression *evaled_args = make_nil();

      SExpression *current = expression->cons.cdr;
      SExpression *current_evaled = evaled_args;
      while (current->is_cons()) {
        SExpression *evaled = eval_sexp(current->cons.car);

        current_evaled->type = SExpression::TYPE_CONS;
        current_evaled->cons.car = evaled;
        current_evaled->cons.cdr = make_nil();
        current_evaled = current_evaled->cons.cdr;

        current = current->cons.cdr;
      }

      result = func->native_procedure(evaled_args);
    } else if (func->is_special_operator()) {
      result = func->native_procedure(expression->cons.cdr);
    } else if (func->is_function()) {
      SExpression *param_list = func->cons.car;
      SExpression *body = func->cons.cdr;

      environment.push_back({});
      Scope &scope = environment.back();

      SExpression *curr_param = param_list;
      SExpression *curr_arg = expression->cons.cdr;
      while (curr_param->is_cons() && curr_arg->is_cons()) {
        const std::string &param_name = *curr_param->cons.car->atom.symbol;
        scope[param_name] = eval_sexp(curr_arg->cons.car);

        curr_param = curr_param->cons.cdr;
        curr_arg = curr_arg->cons.cdr;
      }

      result = progn(body);

      environment.pop_back();
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

SExpression *progn(SExpression *expression) {
  SExpression *result = nullptr;
  while (expression->is_cons()) {
    result = eval_sexp(expression->cons.car);
    expression = expression->cons.cdr;
  }

  if (!result)
    result = make_nil();

  return result;
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

    // The second parameter can either be a symbol to define a variable, or a
    // list to define a function.
    SExpression *second = expression->cons.car;
    if (second->is_symbol()) {
      // Defining a variable

      expression = expression->cons.cdr;
      if (!expression->is_cons())
        break;

      SExpression *value_exp = expression->cons.car;
      SExpression *value = eval_sexp(value_exp);

      // Global scope is used for define
      environment.front()[*second->atom.symbol] = value;
      return value;
    } else if (second->is_cons()) {
      // Defining a function

      // func_list should be a list of symbols, where the first is the function
      // name, and the rest are the parameter names.
      SExpression *func_list = second;

      // func_body is a list of expressions that will be sequentially evaluated
      // when the function is called
      SExpression *func_body = expression->cons.cdr;

      SExpression *func_name_symbol = func_list->cons.car;
      if (!func_name_symbol->is_symbol()) {
        break;
      }

      // Make sure the parameter list is valid
      bool is_valid = true;
      SExpression *curr = func_list->cons.cdr;
      while (!curr->is_nil()) {
        if (!curr->is_cons() || !curr->cons.car->is_symbol()) {
          is_valid = false;
          break;
        }
        curr = curr->cons.cdr;
      }

      if (!is_valid)
        break;

      SExpression *function = make_cons(func_list->cons.cdr, func_body);
      function->type = SExpression::TYPE_FUNCTION;
      environment.front()[*func_name_symbol->atom.symbol] = function;
      return function;
    }
  } while (false);

  std::cout << "ERROR: invalid usage of define" << std::endl;
  return make_nil();
}

SExpression *cond(SExpression *expression) {
  do {
    if (!expression->is_cons())
      break;

    bool invalid = false;

    SExpression *current = expression;
    while (current->is_cons()) {
      SExpression *clause = current->cons.car;
      if (!clause->is_cons()) {
        invalid = true;
        break;
      }

      SExpression *test_form = clause->cons.car;

      SExpression *action = clause->cons.cdr;
      if (action->is_nil()) {
        invalid = true;
        break;
      }

      SExpression *test_result = eval_sexp(test_form);
      if (!test_result->is_nil()) {
        return progn(action);
      }

      current = current->cons.cdr;
    }

    if (invalid)
      break;

    return make_nil();
  } while (false);

  std::cout << "ERROR: invalid usage of cond" << std::endl;
  return make_nil();
}

SExpression *equal(SExpression *expression) {
  do {
    if (!expression->is_cons())
      break;

    SExpression *first = expression->cons.car;

    expression = expression->cons.cdr;
    if (!expression->is_cons())
      break;

    SExpression *second = expression->cons.car;

    if (first->type != second->type)
      return make_nil();

    if (first->type == SExpression::TYPE_ATOM) {
      if (first->atom.type != second->atom.type)
        return make_nil();

      if (first->atom.type == Atom::TYPE_NUMBER) {
        if (first->atom.number == second->atom.number) {
          return make_symbol("t");
        }
      }
    }

    return make_nil();
  } while (false);

  std::cout << "ERROR: = expects two arguments." << std::endl;
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

int main(int argc, char *argv[]) {
  environment.push_back({});
  {
    Scope &globals = environment.back();

    globals["nil"] = make_nil();
    globals["t"] = make_symbol("t");

    globals["progn"] = make_special_operator(progn);
    globals["quote"] = make_special_operator(quote);
    globals["define"] = make_special_operator(define);
    globals["cond"] = make_special_operator(cond);

    globals["="] = make_native_function(equal);
    globals["+"] = make_native_function(add);
    globals["-"] = make_native_function(subtract);
    globals["*"] = make_native_function(multiply);
    globals["/"] = make_native_function(divide);
    globals["list"] = make_native_function(list);
    globals["quit"] = make_native_function(quit);
    globals["eval"] = make_native_function(eval);
  }

  if (argc > 1) {
    // Read from file

    std::string code;

    {
      std::ifstream fs(argv[1]);

      if (!fs.is_open()) {
        std::cout << "ERROR: Could not open file." << std::endl;
        return -1;
      }

      std::getline(fs, code, '\0');
    }

    SExpression *result = execute_string(code);
    std::cout << result->as_string() << std::endl;

    gc_collect(environment.front());
  } else {
    // Repeatedly read from console input

    std::string input;

    while (!should_quit) {
      std::cout << "> ";
      std::getline(std::cin, input);

      SExpression *result = execute_string(input);
      std::cout << result->as_string() << std::endl;

      gc_collect(environment.front());
    }
  }
}
