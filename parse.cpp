#include <iostream>
#include <stack>

#include "parse.hpp"
#include "sexp.hpp"

static bool is_whitespace(char c) { return c == ' ' || c == '\n' || c == '\t'; }

// Returns true if c marks the end of a symbol
static bool is_end_of_symbol(char c) {
  return is_whitespace(c) || c == '(' || c == ')';
}

static bool is_numeric(char c) { return c >= '0' && c <= '9'; }

static int parse_num(const std::string &s, size_t &i) {
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

static std::string parse_sym(const std::string &s, size_t &i) {
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

std::vector<SExpression *> parse_string(const std::string &s) {
  // (1 2 3) -> (1 . (2 . (3 . nil)))
  // ( - Create cons cell
  // 1 - Set car of current cell to 1
  //   - Whitespace: Finish current int
  // 2 - Set cdr of current cell to new cons cell, where its car is set to 2
  // And so on

  // () -> nil

  std::vector<SExpression *> sexps;
  SExpression *current = make_nil();

  // Whenever a list is closed with ')', the previous cons before the start of
  // the list must be the new current expresssion
  std::stack<SExpression *> prev_cons;

  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];

    if (c == ';') {
      // Start of a comment, ignore the rest of the characters until there is a
      // newline.

      size_t newline_pos = s.find('\n', i + 1);
      if (newline_pos == std::string::npos) {
        // Newline not found, just break out of the loop
        break;
      } else {
        i = newline_pos;
        continue;
      }
    }

    if (is_whitespace(c))
      continue;

    if (c == ')') {
      if (prev_cons.empty()) {
        std::cout << "ERROR: Unmatched right parenthesis." << std::endl;
        return sexps;
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
        // The root list just got finished, so the current expression is
        // completed.
        sexps.push_back(current);
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
        // A list is not being created, so whatever expression just got made is
        // already completed.
        sexps.push_back(current);
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
    return sexps;
  }

  return sexps;
}
