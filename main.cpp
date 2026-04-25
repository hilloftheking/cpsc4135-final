#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>

struct Atom {
  enum Type { TYPE_NUMBER, TYPE_SYMBOL };
  Type type;
  union {
    int number; // Will be a double in the future probably
    std::string *symbol;
  };

  Atom() {}

  ~Atom() {
    if (type == TYPE_SYMBOL) {
      delete symbol;
      symbol = nullptr;
    }
  }

  std::string as_string() const {
    if (type == TYPE_NUMBER) {
      return std::to_string(number);
    } else {
      return *symbol;
    }
  }
};

// Forward declaration for ConsCell
struct SExpression;

// Basically a linked list. car is the head, and cdr is rest. An SExpression can
// be a ConsCell, so it is recursive.
struct ConsCell {
  SExpression *car;
  SExpression *cdr;
};

struct SExpression {
  enum Type { TYPE_NIL, TYPE_ATOM, TYPE_CONS };
  Type type;
  union {
    Atom atom;
    ConsCell cons;
  };

  SExpression() { type = TYPE_NIL; }

  ~SExpression() {
    if (type == TYPE_CONS) {
      delete cons.car;
      delete cons.cdr;
    }
    type = TYPE_NIL;
  }

  bool is_nil() const { return type == TYPE_NIL; }
  bool is_atom() const { return type == TYPE_ATOM; }
  bool is_cons() const { return type == TYPE_CONS; }

  bool is_number() const { return is_atom() && atom.type == Atom::TYPE_NUMBER; }
  bool is_symbol() const { return is_atom() && atom.type == Atom::TYPE_SYMBOL; }

  // Returns true if the expression is not a cons cell or the cons cell is full
  bool is_full() const {
    return type != TYPE_CONS || (!cons.car->is_nil() && !cons.cdr->is_nil());
  }
};

// A function that takes an expression and returns a new one
using LispFunction = SExpression *(*)(SExpression *);

std::unordered_map<std::string, LispFunction> functions;

bool should_quit = false;

SExpression *make_nil() { return new SExpression; }

SExpression *make_number(int num) {
  SExpression *sexp = new SExpression;
  sexp->type = SExpression::TYPE_ATOM;
  sexp->atom.type = Atom::TYPE_NUMBER;
  sexp->atom.number = num;
  return sexp;
}

SExpression *make_symbol(const std::string &sym) {
  SExpression *sexp = new SExpression;
  sexp->type = SExpression::TYPE_ATOM;
  sexp->atom.type = Atom::TYPE_SYMBOL;
  sexp->atom.symbol = new std::string(sym);
  return sexp;
}

SExpression *make_cons(SExpression *car, SExpression *cdr) {
  SExpression *sexp = new SExpression;
  sexp->type = SExpression::TYPE_CONS;
  sexp->cons.car = car;
  sexp->cons.cdr = cdr;
  return sexp;
}

void print_sexp(const SExpression &sexp, bool is_first = true,
                bool new_line = true) {
  bool start_of_list = false;
  if (sexp.is_cons() && is_first) {
    start_of_list = true;
    std::cout << '(';
  }

  if (!is_first) {
    std::cout << ' ';
  }

  if (sexp.is_atom()) {
    std::cout << sexp.atom.as_string();
  } else if (sexp.is_cons()) {
    print_sexp(*sexp.cons.car, true, false);
    // If cdr is nil, then the list is terminated
    if (!sexp.cons.cdr->is_nil()) {
      print_sexp(*sexp.cons.cdr, false, false);
    }
  } else if (sexp.is_nil()) {
    std::cout << "nil";
  }

  if (start_of_list) {
    std::cout << ')';
  }

  if (new_line) {
    std::cout << std::endl;
  }
}

// Prints a longer dotted representation that is more accurate to the cons
// structure
void print_dotted(const SExpression &sexp, bool new_line = true) {
  if (sexp.is_atom()) {
    std::cout << sexp.atom.as_string();
  } else if (sexp.is_cons()) {
    std::cout << '(';
    print_dotted(*sexp.cons.car, false);
    std::cout << " . ";
    print_dotted(*sexp.cons.cdr, false);
    std::cout << ')';
  } else if (sexp.is_nil()) {
    std::cout << "nil";
  }

  if (new_line) {
    std::cout << std::endl;
  }
}

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
    SExpression *result = make_cons(expression->cons.car, expression->cons.cdr);
    expression->type = SExpression::TYPE_NIL;
    return result;
  }
}

SExpression *quit(SExpression *expression) {
  should_quit = true;
  return make_nil();
}

SExpression *evaluate(SExpression *expression) {
  if (expression->is_cons()) {
    // This expression is the start of a list

    SExpression *first = expression->cons.car;
    if (!first->is_symbol()) {
      // Functions are associated with a symbol
      std::cout << "ERROR: Invalid function name, expected symbol."
                << std::endl;
      delete expression;
      return make_nil();
    }

    auto it = functions.find(*first->atom.symbol);
    if (it == functions.end()) {
      // Symbol does not have a corresponding function
      std::cout << "ERROR: Function " << *first->atom.symbol << " not found."
                << std::endl;
      delete expression;
      return make_nil();
    }

    LispFunction function = it->second;

    SExpression *current = expression->cons.cdr;
    while (current->is_cons()) {
      // Each car can be replaced with an evaluated version :)
      current->cons.car = evaluate(current->cons.car);
      current = current->cons.cdr;
    }

    SExpression *result = function(expression->cons.cdr);
    delete expression;
    return result;
  } else {
    return expression;
  }
}

SExpression *execute_string(const std::string &s) {
  // (1 2 3) -> (1 . (2 . (3 . nil)))
  // ( - Create cons cell
  // 1 - Set car of current cell to 1
  //   - Whitespace: Finish current int
  // 2 - Set cdr of current cell to new cons cell, where its car is set to 2
  // And so on

  // TODO:
  // () -> nil

  // TODO: more error handling

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
        delete current;
        return make_nil();
      }

      // The current chain of cons cells has to end. This means that the
      // previous cons before the start of the current cons chain is now the
      // current expression.
      current = prev_cons.top();
      prev_cons.pop();

      if (prev_cons.empty()) {
        // The root list just got finished, so it should be evaluated now
        current = evaluate(current);
      }

      continue;
    }

    // This is the new expression that will be parsed and inserted into the
    // current expression
    SExpression *new_expression = nullptr;

    if (c == '(') {
      // Start of list
      new_expression = make_cons(make_nil(), make_nil()); // Empty cons cell
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

    if (current->is_full()) {
      // Either not inside of a cons cell, or the cons cell is full.
      // So the current expression should just be overwritten.
      delete current;
      current = new_expression;
    } else {
      // Current expression is a cons cell that the new expression must be
      // inserted into.
      if (current->cons.car->is_nil()) {
        delete current->cons.car;
        current->cons.car = new_expression;
      } else {
        // The cdr of the current expression, which is a cons, was nil, but it
        // will now be a new cons and the current expression.
        // This new cons will have the new expression and nil.
        current = current->cons.cdr;
        current->type = SExpression::TYPE_CONS;
        current->cons.car = new_expression;
        current->cons.cdr = make_nil();
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
    delete current;
    return make_nil();
  }

  // TODO: Once an expression has been finished, if it is a list, then it should
  // be executed since the first expression in it should be a function

  return current;
}

int main() {
  functions["+"] = add;
  functions["-"] = subtract;
  functions["*"] = multiply;
  functions["/"] = divide;
  functions["list"] = list;
  functions["quit"] = quit;

  std::string input;

  while (!should_quit) {
    std::cout << "> ";
    std::getline(std::cin, input);

    SExpression *result = execute_string(input);
    print_sexp(*result);
    // print_dotted(*result);
    delete result;
  }
}
