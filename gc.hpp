#pragma once

struct SExpression;

// Registers an expression with the garbage collector.
void gc_add_sexp(SExpression *sexp);

// Runs the garbage collector, freeing any expressions that are unneeded.
void gc_collect();
