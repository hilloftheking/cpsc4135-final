#include <iostream>
#include <string>

// For now, an atom is just a number.
using Atom = int;

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
    std::cout << sexp.atom;
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

int main() {
  // (1 (2 3 4) 5)
  auto *a = new SExpression(
      new SExpression(1),
      new SExpression(
          new SExpression(new SExpression(2),
                          new SExpression(new SExpression(3),
                                          new SExpression(new SExpression(4),
                                                          new SExpression))),
          new SExpression(new SExpression(5), new SExpression)));
  print_sexp(*a);
}
