#include "repl.hpp"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"

#define REPL_PARAM_SLOTS 3
#define REPL_TOKEN ":"

void Repl::process(char* input) {
  // strip line endings from input
  size_t in_len = strlen(input);
  for (size_t i = in_len-1; i >=0 ; i--) {
    if (input[i] == 0) continue;
    if (isspace(input[i])) {
      input[i] = 0;
    } else {
      break;
    }
  }
  
  char* slots[REPL_PARAM_SLOTS] = {NULL};
  size_t i = 0;
  char* t = strtok(input, REPL_TOKEN);
  while (t != NULL && i < REPL_PARAM_SLOTS) {
    slots[i] = t;
    i++;
    t = strtok(NULL, REPL_TOKEN);
  }

  // filter out any inputs that dont start with "cmd:"
  if (i < 2 || (strcmp(slots[0], "cmd") != 0)) {
    log_line("unknown command, start with 'cmd:'");
    return;
  }

  // check for manual reset to bootloader mode
  bool consumed = false;
  if (strncmp(slots[1], "boot", 4) == 0) {
    log_line("resetting to usb boot mode");
    consumed = true;
    reboot_to_uf2(0, 0);
    // check for setting of LED brightness
  } else if (i >= 2 && strcmp(slots[1], "brightness") == 0 && slots[2]) {
    int brightness = atoi(slots[2]);
    if (brightness > 0) {
      if (brightness < 100) {
        persistence->setLedBrightness((float)brightness / 100.0f);
        log_line("set brightness to: %d%%", brightness);
      } else {
        log_line("please enter brightness integer between 1 - 100");
      }
    } else {
      log_line("invalid input, usage: cmd:brightness:[1-100]");
    }
    consumed = true;
  } else if (i >= 2 && strcmp(slots[1], "raw_hid") == 0 && slots[2]) {
    if (strcmp(slots[2], "on") == 0) {
      persistence->setRawHidLogsEnabled(true);
      log_line("raw hid logging enabled");
    } else if (strcmp(slots[2], "off") == 0){
      persistence->setRawHidLogsEnabled(false);
      log_line("raw hid logging disabled");
    } else {
      log_line("invalid input, usage: cmd:raw_hid:[on|off]");
    }
    consumed = true;
  }

  if (!consumed) {
    log_line("unknown command %s", slots[1]);
  }
}