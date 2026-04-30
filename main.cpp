#include <fstream>
#include <iostream>
#include <string>

#include "environment.hpp"
#include "gc.hpp"
#include "parse.hpp"
#include "sexp.hpp"

int main(int argc, char *argv[]) {
  create_globals();

  if (argc > 1) {
    // Read from file

    std::string code;

    {
      std::ifstream fs(argv[1]);

      if (!fs.is_open()) {
        std::cout << "ERROR: Could not open file." << std::endl;
        return -1;
      }

      std::getline(fs, code, '\0');
    }

    SExpression *result = eval_sexps(parse_string(code));
    std::cout << result->as_string() << std::endl;

    gc_collect();
  } else {
    // Repeatedly read from console input

    std::string input;

    while (!should_quit()) {
      std::cout << "> ";
      std::getline(std::cin, input);

      SExpression *result = eval_sexps(parse_string(input));
      std::cout << result->as_string() << std::endl;

      gc_collect();
    }
  }
}
