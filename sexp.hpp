#pragma once

#include <string>

struct Atom {
  enum Type { TYPE_NUMBER, TYPE_SYMBOL };
  Type type;
  union {
    int number; // Will be a double in the future probably
    std::string *symbol;
  };
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
  ~SExpression();

  bool is_nil() const { return type == TYPE_NIL; }
  bool is_atom() const { return type == TYPE_ATOM; }
  bool is_cons() const { return type == TYPE_CONS; }

  bool is_number() const { return is_atom() && atom.type == Atom::TYPE_NUMBER; }
  bool is_symbol() const { return is_atom() && atom.type == Atom::TYPE_SYMBOL; }

  // Returns true if the expression is not a cons cell or the cons cell is full
  bool is_full() const {
    return type != TYPE_CONS || (!cons.car->is_nil() && !cons.cdr->is_nil());
  }

  std::string as_string() const;

private:
  std::string inner_as_string() const;
};

SExpression *make_nil();

SExpression *make_number(int num);

SExpression *make_symbol(const std::string &sym);

SExpression *make_cons(SExpression *car, SExpression *cdr);

SExpression *make_copy(SExpression *sexpr);
