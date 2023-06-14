#include "persistence.hpp"

class Repl {
 private:
  IPersistence *persistence;

 public:
  Repl(IPersistence *persistence) { this->persistence = persistence; }
  void process(char *input);
};