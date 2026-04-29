#include <stack>
#include <vector>

#include "gc.hpp"
#include "sexp.hpp"

static std::vector<SExpression *> expressions;

void gc_add_sexp(SExpression *sexp) {
  sexp->gc_marked = false;
  expressions.push_back(sexp);
}

void gc_collect(const std::unordered_map<std::string, SExpression *> &globals) {
  // Start from the globals, and mark any reachable expression. Then, free all
  // the expressions that are not marked.

  // Right now this will not work for any cycles.

  std::stack<SExpression *> need_to_check;

  for (auto &[_, sexp] : globals) {
    sexp->gc_marked = true;
    if (sexp->is_cons()) {
      need_to_check.push(sexp->cons.car);
      need_to_check.push(sexp->cons.cdr);
    }
  }

  while (!need_to_check.empty()) {
    SExpression *sexp = need_to_check.top();
    need_to_check.pop();

    sexp->gc_marked = true;
    if (sexp->is_cons()) {
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
