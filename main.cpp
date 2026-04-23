#include <iostream>
#include <stack>
#include <string>

struct Atom {
  enum Type { TYPE_NUMBER, TYPE_SYMBOL };
  Type type;
  union {
    int number; // Will be a double in the future probably
    std::string *symbol;
  };

  Atom(int num) {
    type = TYPE_NUMBER;
    number = num;
  }

  Atom(const std::string &sym) {
    type = TYPE_SYMBOL;
    symbol = new std::string(sym);
  }

  Atom &operator=(const Atom &other) {
    type = other.type;
    if (type == TYPE_NUMBER) {
      number = other.number;
    } else {
      symbol = new std::string(*other.symbol);
    }
    return *this;
  }

  Atom(const Atom &other) { *this = other; }

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

class SExpression {
public:
  enum Type { TYPE_NIL, TYPE_ATOM, TYPE_CONS };
  Type type;
  union {
    Atom atom;
    ConsCell cons;
  };

  SExpression() { type = TYPE_NIL; }

  SExpression(const Atom &a) {
    type = TYPE_ATOM;
    atom = a;
  }

  SExpression(SExpression *car, SExpression *cdr) {
    type = TYPE_CONS;
    cons.car = car;
    cons.cdr = cdr;
  }

  ~SExpression() {
    if (type == TYPE_CONS) {
      delete cons.car;
      delete cons.cdr;
    }
    type = TYPE_NIL;
  }

  bool is_nil() { return type == TYPE_NIL; }
};

void print_sexp(const SExpression &sexp, bool is_first = true,
                bool new_line = true) {
  bool start_of_list = false;
  if (sexp.type == SExpression::TYPE_CONS && is_first) {
    start_of_list = true;
    std::cout << '(';
  }

  if (!is_first) {
    std::cout << ' ';
  }

  if (sexp.type == SExpression::TYPE_ATOM) {
    std::cout << sexp.atom.as_string();
  } else if (sexp.type == SExpression::TYPE_CONS) {
    print_sexp(*sexp.cons.car, true, false);
    // If cdr is nil, then the list is terminated
    if (sexp.cons.cdr->type != SExpression::TYPE_NIL) {
      print_sexp(*sexp.cons.cdr, false, false);
    }
  } else if (sexp.type == SExpression::TYPE_NIL) {
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
  if (sexp.type == SExpression::TYPE_ATOM) {
    std::cout << sexp.atom.as_string();
  } else if (sexp.type == SExpression::TYPE_CONS) {
    std::cout << '(';
    print_dotted(*sexp.cons.car, false);
    std::cout << " . ";
    print_dotted(*sexp.cons.cdr, false);
    std::cout << ')';
  } else if (sexp.type == SExpression::TYPE_NIL) {
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

SExpression *execute(const std::string &s) {
  // (1 2 3) -> (1 . (2 . (3 . nil)))
  // ( - Create cons cell
  // 1 - Set car of current cell to 1
  //   - Whitespace: Finish current int
  // 2 - Set cdr of current cell to new cons cell, where its car is set to 2
  // And so on

  // TODO:
  // () -> nil

  SExpression *current = new SExpression();
  SExpression *start = current;

  // Whenever a list is closed with ')', the previous cons before the start of
  // the list must be the new current expresssion
  std::stack<SExpression *> prev_cons;

  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];

    // TODO: going to clean up all this copy and paste

    if (is_numeric(c)) {
      // parse_num will modify i to be right after the number
      int num = parse_num(s, i);
      if (current->type != SExpression::TYPE_CONS) {
        // Not inside a cons, just overwrite the current expression
        current->type = SExpression::TYPE_ATOM;
        current->atom = num;
      } else {
        if (current->cons.car->is_nil()) {
          current->cons.car->type = SExpression::TYPE_ATOM;
          current->cons.car->atom = num;
        } else {
          // The cdr of the current expression, which is a cons, was nil, but it
          // will now be a new cons and the current expression.
          // This new cons will have the number and nil.
          current = current->cons.cdr;
          current->type = SExpression::TYPE_CONS;
          current->cons.car = new SExpression(num);
          current->cons.cdr = new SExpression;
        }
      }

      // After parsing a num, i is set to immediately after the number within
      // the string. So it must be decremented since it will be incremented
      // again.
      i--;
    } else if (c == '(') {
      if (current->type != SExpression::TYPE_CONS) {
        // Not inside a cons, just overwrite the current expression
        prev_cons.push(nullptr);
        current->type = SExpression::TYPE_CONS;
        current->cons.car = new SExpression;
        current->cons.cdr = new SExpression;
      } else {
        if (current->cons.car->is_nil()) {
          // The car of the current expression was nil, so it will now become an
          // empty cons, which will be the new current expression.
          prev_cons.push(current);
          current = current->cons.car;
          current->type = SExpression::TYPE_CONS;
          current->cons.car = new SExpression;
          current->cons.cdr = new SExpression;
        } else {
          // The cdr of the current expression, which is a cons, was nil, but it
          // will now be a new cons and the current expression.
          // This new cons will also have an empty cons inside its car and nil
          // in its cdr.
          current = current->cons.cdr;
          current->type = SExpression::TYPE_CONS;
          current->cons.car =
              new SExpression(new SExpression, new SExpression); // empty cons
          current->cons.cdr = new SExpression;
          // Since a new cons was created, now IT is the new current
          // expression
          prev_cons.push(current);
          current = current->cons.car;
        }
      }
    } else if (c == ')') {
      // The current chain of cons cells has to end. This means that the
      // previous cons before the start of the current cons chain is now the
      // current expression.
      current = prev_cons.top();
      prev_cons.pop();
    } else if (is_whitespace(c)) {
      continue;
    } else {
      std::string sym = parse_sym(s, i);
      if (current->type != SExpression::TYPE_CONS) {
        // Not inside a cons, just overwrite the current expression
        current->type = SExpression::TYPE_ATOM;
        current->atom = sym;
      } else {
        if (current->cons.car->is_nil()) {
          current->cons.car->type = SExpression::TYPE_ATOM;
          current->cons.car->atom = sym;
        } else {
          // The cdr of the current expression, which is a cons, was nil, but it
          // will now be a new cons and the current expression.
          // This new cons will have the symbol and nil.
          current = current->cons.cdr;
          current->type = SExpression::TYPE_CONS;
          current->cons.car = new SExpression(sym);
          current->cons.cdr = new SExpression;
        }
      }

      // After parsing a symbol, i is set to immediately after the symbol within
      // the string. So it must be decremented since it will be incremented
      // again.
      i--;
    }
  }

  // TODO: Once an expression has been finished, if it is a list, then it should
  // be executed since the first expression in it should be a function

  return start;
}

// A few temporary helper functions for debugging
static SExpression *cons(SExpression *car, SExpression *cdr) {
  return new SExpression(car, cdr);
}
static SExpression *atom(int n) { return new SExpression(n); }
static SExpression *nil() { return new SExpression; }

int main() {
  // (1 (2 3 4) 5)
  auto *a =
      cons(atom(1), cons(cons(atom(2), cons(atom(3), cons(atom(4), nil()))),
                         cons(atom(5), nil())));
  print_sexp(*a);
  print_dotted(*a);
  delete a;

  std::cout << std::endl;

  std::cout << ">";
  std::string input;
  std::getline(std::cin, input);
  SExpression *out = execute(input);
  print_sexp(*out);
  print_dotted(*out);
  delete out;
}
