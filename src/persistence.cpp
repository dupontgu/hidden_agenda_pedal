#include <stdio.h>
#include "persistence.h"

static bool initialized = false;

static settings_t default_settings = {
    .version = 0,
    .active_fx_slot = 0,
    .led_brightness = 1.0,
    .slot_colors = {
        0xFFFF4000,
        0xFF4000FF,
        0xFF00FF40,
        0xFFAA0070
    }
};

void init_persistence() {
  if (!initialized) {
    initialized = true;
    // TODO init EEPROM
  }
}

settings_t read_settings_internal() {
  // TODO read from EEPROM
  return default_settings;
}

settings_t migrate(uint8_t from, uint8_t to, void *persisted) {
  // hopefully I never actually use this
  (void)from;
  (void)to;
  (void)persisted;
  return default_settings;
}

settings_t read_settings_from_persistence() {
  settings_t persisted = read_settings_internal();
  if (persisted.version != default_settings.version) {
    persisted =
        migrate(persisted.version, default_settings.version, &persisted);
    write_settings_to_persistence(persisted);
  }
  return persisted;
}

void write_settings_to_persistence(settings_t settings) { (void)settings; }