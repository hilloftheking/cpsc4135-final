#pragma once

#include <string>

struct Atom {
  enum Type { TYPE_NIL, TYPE_NUMBER, TYPE_SYMBOL };
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

// A function that takes an expression and returns a new one
// This can be exposed as either a function or special operator
using NativeProcedure = SExpression *(*)(SExpression *);

struct SExpression {
  enum Type {
    TYPE_ATOM,            // Holds an atom
    TYPE_CONS,            // Holds a cons cell
    TYPE_NATIVE_FUNCTION, // Holds a native function inside of native_procedure.
                          // Each argument should be evaluated before being
                          // called. The returned expression should not be
                          // evaluated.
    TYPE_SPECIAL_OPERATOR, // Holds a native special operator function inside of
                           // native_procedure. Neither the arguments nor the
                           // returned expression should be evaluated.
    TYPE_FUNCTION // Holds a function defined with expressions. The function is
                  // actually stored in the cons cell, where car is the list of
                  // parameter names, and cdr is the body of the function.
  };
  Type type;
  union {
    Atom atom;
    ConsCell cons;
    NativeProcedure native_procedure;
  };

  // Used by the garbage collector to know what expressions can be freed.
  bool gc_marked;

  SExpression();
  ~SExpression();

  bool is_atom() const { return type == TYPE_ATOM; }
  bool is_cons() const { return type == TYPE_CONS; }
  bool is_native_function() const { return type == TYPE_NATIVE_FUNCTION; }
  bool is_special_operator() const { return type == TYPE_SPECIAL_OPERATOR; }
  bool is_function() const { return type == TYPE_FUNCTION; }

  bool is_nil() const { return is_atom() && atom.type == Atom::TYPE_NIL; }
  bool is_number() const { return is_atom() && atom.type == Atom::TYPE_NUMBER; }
  bool is_symbol() const { return is_atom() && atom.type == Atom::TYPE_SYMBOL; }

  std::string as_string() const;

private:
  std::string inner_as_string() const;
};

SExpression *make_nil();

SExpression *make_number(int num);

SExpression *make_symbol(const std::string &sym);

SExpression *make_cons(SExpression *car, SExpression *cdr);

SExpression *make_native_function(NativeProcedure proc);

SExpression *make_special_operator(NativeProcedure proc);
