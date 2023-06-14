#include "repl.hpp"

#include <stdlib.h>
#include <string.h>

#include "util.h"

#define REPL_PARAM_SLOTS 3
#define REPL_TOKEN ":"

void Repl::process(char* input) {
  char* slots[REPL_PARAM_SLOTS] = {NULL};
  size_t i = 0;
  char* t = strtok(input, REPL_TOKEN);
  while (t != NULL && i < REPL_PARAM_SLOTS) {
    slots[i] = t;
    i++;
    t = strtok(NULL, REPL_TOKEN);
  }

  if (i < 2 || (strcmp(slots[0], "cmd") != 0)) {
    log_line("unknown command, start with 'cmd:'");
    return;
  }

  bool consumed = false;
  if (strncmp(slots[1], "boot", 4) == 0) {
    log_line("resetting to usb boot mode");
    consumed = true;
    reboot_to_uf2(0, 0);
  } else if (i >= 2 && strcmp(slots[1], "brightness") == 0) {
    int brightness = atoi(slots[2]);
    if (brightness > 0) {
      if (brightness < 100) {
        persistence->setLedBrightness((float)brightness / 100.0f);
        log_line("set brightness to: %d%%", brightness);
      } else {
        log_line("please enter brightness integer between 1 - 100");
      }
      consumed = true;
    }
  }

  if (!consumed) {
    log_line("unknown command %s", slots[1]);
  }
}