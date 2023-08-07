#include "hid_output.hpp"
#include "persistence.hpp"

class Repl {
 private:
  IPersistence *persistence;
  IHIDOutput *hid_output;

 public:
  Repl(IPersistence *persistence, IHIDOutput *hid_output) {
    this->persistence = persistence;
    this->hid_output = hid_output;
  }
  void process(char *input);
};