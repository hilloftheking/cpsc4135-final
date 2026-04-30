#include <stack>
#include <vector>

#include "environment.hpp"
#include "gc.hpp"
#include "sexp.hpp"

static std::vector<SExpression *> expressions;

void gc_add_sexp(SExpression *sexp) {
  sexp->gc_marked = false;
  expressions.push_back(sexp);
}

void gc_collect() {
  // Starting from expressions within the current environment scopes, mark all
  // reachable expressions. Then free the expressions that are not marked.

  // Right now this will not work for any cycles.

  std::stack<SExpression *> need_to_check;

  for (auto &scope : environment) {
    for (auto &[_, sexp] : scope) {
      sexp->gc_marked = true;
      if (sexp->is_cons() || sexp->is_function()) {
        need_to_check.push(sexp->cons.car);
        need_to_check.push(sexp->cons.cdr);
      }
    }
  }

  while (!need_to_check.empty()) {
    SExpression *sexp = need_to_check.top();
    need_to_check.pop();

    sexp->gc_marked = true;
    if (sexp->is_cons() || sexp->is_function()) {
      if (sexp->cons.car)
        need_to_check.push(sexp->cons.car);
      if (sexp->cons.cdr)
        need_to_check.push(sexp->cons.cdr);
    }
  }

  auto it = expressions.begin();
  while (it != expressions.end()) {
    SExpression *sexp = *it;

    if (!sexp->gc_marked) {
      // If the expression was not marked, it is not reachable.
      delete sexp;
      it = expressions.erase(it);
    } else {
      // Reset marked back to false
      sexp->gc_marked = false;
      it++;
    }
  }
}
