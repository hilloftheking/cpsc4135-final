#include <iostream>

#include "environment.hpp"
#include "sexp.hpp"

std::vector<Scope> environment;

static bool quit_requested = false;

static SExpression *progn(SExpression *sexp) {
  SExpression *result = nullptr;
  while (sexp->is_cons()) {
    result = eval_sexp(sexp->cons.car);
    sexp = sexp->cons.cdr;
  }

  if (!result)
    result = make_nil();

  return result;
}

static SExpression *quote(SExpression *sexp) {
  if (!sexp->is_cons()) {
    std::cout << "ERROR: quote expects one argument." << std::endl;
    return make_nil();
  }

  return sexp->cons.car;
}

static SExpression *define(SExpression *sexp) {
  do {
    if (!sexp->is_cons())
      break;

    // The second parameter can either be a symbol to define a variable, or a
    // list to define a function.
    SExpression *second = sexp->cons.car;
    if (second->is_symbol()) {
      // Defining a variable

      sexp = sexp->cons.cdr;
      if (!sexp->is_cons())
        break;

      SExpression *value_form = sexp->cons.car;
      SExpression *value = eval_sexp(value_form);

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
      SExpression *func_body = sexp->cons.cdr;

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

static SExpression *cond(SExpression *sexp) {
  do {
    if (!sexp->is_cons())
      break;

    bool invalid = false;

    SExpression *current = sexp;
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

static SExpression *equal(SExpression *sexp) {
  do {
    if (!sexp->is_cons())
      break;

    SExpression *first = sexp->cons.car;

    sexp = sexp->cons.cdr;
    if (!sexp->is_cons())
      break;

    SExpression *second = sexp->cons.car;

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

static SExpression *add(SExpression *sexp) {
  int result = 0;
  while (sexp->is_cons()) {
    SExpression *car = sexp->cons.car;
    if (car->is_number()) {
      result += car->atom.number;
    }

    sexp = sexp->cons.cdr;
  }

  return make_number(result);
}

static SExpression *subtract(SExpression *sexp) {
  int result = 0;
  bool is_first = true;
  while (sexp->is_cons()) {
    SExpression *car = sexp->cons.car;
    if (car->is_number()) {
      if (is_first) {
        is_first = false;
        result = car->atom.number;
      } else {
        result -= car->atom.number;
      }
    }

    sexp = sexp->cons.cdr;
  }

  return make_number(result);
}

static SExpression *multiply(SExpression *sexp) {
  int result = 1;
  while (sexp->is_cons()) {
    SExpression *car = sexp->cons.car;
    if (car->is_number()) {
      result *= car->atom.number;
    }

    sexp = sexp->cons.cdr;
  }

  return make_number(result);
}

static SExpression *divide(SExpression *sexp) {
  int result = 0;
  bool is_first = true;
  while (sexp->is_cons()) {
    SExpression *car = sexp->cons.car;
    if (car->is_number()) {
      if (is_first) {
        is_first = false;
        result = car->atom.number;
      } else {
        result /= car->atom.number;
      }
    }

    sexp = sexp->cons.cdr;
  }

  return make_number(result);
}

static SExpression *list(SExpression *sexp) {
  if (sexp->is_nil()) {
    return make_nil();
  } else {
    return sexp;
  }
}

static SExpression *quit(SExpression *sexp) {
  quit_requested = true;
  return make_nil();
}

static SExpression *eval(SExpression *sexp) {
  if (!sexp->is_cons() || !sexp->cons.cdr->is_nil()) {
    std::cout << "ERROR: eval expects one argument." << std::endl;
    return make_nil();
  }

  return eval_sexp(sexp->cons.car);
}

static SExpression *print(SExpression *sexp) {
  if (sexp->is_cons()) {
    std::cout << sexp->cons.car->as_string() << std::endl;
  }
  return make_nil();
}

bool should_quit() { return quit_requested; }

SExpression *eval_sexp(SExpression *sexp) {
  if (sexp->is_cons()) {
    // This expression is the start of a list

    // Eval the first element of the list to get the function expression
    SExpression *func = eval_sexp(sexp->cons.car);

    SExpression *result;

    if (func->is_native_function()) {
      // Create a new list of evaluated arguments
      SExpression *evaled_args = make_nil();

      SExpression *current = sexp->cons.cdr;
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
      result = func->native_procedure(sexp->cons.cdr);
    } else if (func->is_function()) {
      SExpression *param_list = func->cons.car;
      SExpression *body = func->cons.cdr;

      environment.push_back({});
      Scope &scope = environment.back();

      SExpression *curr_param = param_list;
      SExpression *curr_arg = sexp->cons.cdr;
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
  } else if (sexp->is_symbol()) {
    const std::string &sym = *sexp->atom.symbol;

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
    return sexp;
  }
}

SExpression *eval_sexps(std::vector<SExpression *> expressions) {
  SExpression *result = nullptr;

  for (auto *sexp : expressions) {
    result = eval_sexp(sexp);
  }

  if (result == nullptr)
    result = make_nil();

  return result;
}

void create_globals() {
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
    globals["print"] = make_native_function(print);
  }
}
