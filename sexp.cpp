#include "sexp.hpp"

SExpression::SExpression() {
  type = TYPE_ATOM;
  atom.type = Atom::TYPE_NIL;
}

SExpression::~SExpression() {
  if (type == TYPE_CONS) {
    delete cons.car;
    delete cons.cdr;
  } else if (type == TYPE_ATOM) {
    if (atom.type == Atom::TYPE_SYMBOL) {
      delete atom.symbol;
    }
  }
  type = TYPE_ATOM;
  atom.type = Atom::TYPE_NIL;
}

std::string SExpression::as_string() const {
  // If as_string is called on a cons cell, it must be the start of a list, so
  // it is printed with parentheses. Then inside of inner_as_string, ONLY a
  // cons cell inside the car of another cons cell will be printed with
  // parentheses.

  if (is_cons()) {
    return "(" + inner_as_string() + ")";
  } else {
    return inner_as_string();
  }
}

std::string SExpression::inner_as_string() const {
  if (is_nil()) {
    return "nil";
  } else if (is_number()) {
    return std::to_string(atom.number);
  } else if (is_symbol()) {
    return *atom.symbol;
  } else if (is_cons()) {
    std::string result = "";

    if (cons.car->is_cons()) {
      // Cons cell inside the car of another cons cell means a nested list
      result += "(" + cons.car->inner_as_string() + ")";
    } else {
      result += cons.car->inner_as_string();
    }

    // A cdr of nil marks the end of the list
    if (!cons.cdr->is_nil()) {
      result += " " + cons.cdr->inner_as_string();
    }

    return result;
  } else {
    return "?"; // Unimplemented?
  }
}

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

SExpression *make_copy(SExpression *sexpr) {
  if (sexpr->is_nil()) {
    return make_nil();
  } else if (sexpr->is_number()) {
    return make_number(sexpr->atom.number);
  } else if (sexpr->is_symbol()) {
    return make_symbol(*sexpr->atom.symbol);
  } else {
    // TODO
    return make_nil();
  }
}
