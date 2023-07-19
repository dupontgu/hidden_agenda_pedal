#include "i2c_persistence.hpp"

#include <stdio.h>
#include <string.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define PERSISTENCE_EEPROM_ADDR 0x50
#define PERSISTENCE_EEPROM_SDA_PIN 6
#define PERSISTENCE_EEPROM_SCL_PIN 7
#define PERSISTENCE_EEPROM_START_REG 0
#define PERSISTENCE_I2C i2c1

static bool initialized = false;

static settings_t default_settings = {
    // VERSION MUST ALWAYS STAY FIRST!!!!!
    .version = 1,
    .active_fx_slot = 0,
    .report_parse_mode = 0,
    .log_mode = PERSISTENCE_LOG_OFF,
    .led_brightness = 1.0,
    .slot_colors = {0xFFFF4000, 0xFF4000FF, 0xFF00FF40, 0xFFAA0070}};

settings_t active_settings = default_settings;

static void eeprom_polling() {
  uint8_t i = 0;
  uint8_t tmp;
  int8_t ret;
  while (1) {
    ret = i2c_read_blocking(PERSISTENCE_I2C, PERSISTENCE_EEPROM_ADDR, &tmp, 1,
                            false);
    if (ret > 0) break;
    sleep_ms(1);
    i++;
  }
}

// write without Repeated Start condition
// adapted from https://github.com/Yudetamago-AM/pico-eeprom-i2c
int i2c_write_nors_blocking(const uint8_t *src, int len) {
  // send without a Repeated Start conditions between chunks
  // code below originally comes from
  // "pico-sdk/src/rp2_common/hardware_i2c/i2c.c" which is under BSD 3-Clause
  // "New" or "Reviced" License:
  /*
  Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors
  may be used to endorse or promote products derived from this software without
  specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */
  bool abort = false;
  int byte_ctr;
  i2c_inst_t *i2c = PERSISTENCE_I2C;
  for (byte_ctr = 0; byte_ctr < len; ++byte_ctr) {
    bool last = byte_ctr == len - 1;

    i2c->hw->data_cmd = bool_to_bit(last) << I2C_IC_DATA_CMD_STOP_LSB | *src++;

    // Wait until the transmission of the address/data from the internal
    // shift register has completed. For this to function correctly, the
    // TX_EMPTY_CTRL flag in IC_CON must be set. The TX_EMPTY_CTRL flag
    // was set in i2c_init.
    do {
      tight_loop_contents();
    } while (!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_EMPTY_BITS));

    // If there was a timeout, don't attempt to do anything else.
    bool abort_reason = i2c->hw->tx_abrt_source;
    if (abort_reason) {
      // Note clearing the abort flag also clears the reason, and
      // this instance of flag is clear-on-read! Note also the
      // IC_CLR_TX_ABRT register always reads as 0.
      i2c->hw->clr_tx_abrt;
      abort = true;
    }

    if (abort || last) {
      // If the transaction was aborted or if it completed
      // successfully wait until the STOP condition has occured.

      // TODO Could there be an abort while waiting for the STOP
      // condition here? If so, additional code would be needed here
      // to take care of the abort.
      do {
        tight_loop_contents();
      } while (!(i2c->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS));
    }

    // Note the hardware issues a STOP automatically on an abort condition.
    // Note also the hardware clears RX FIFO as well as TX on abort,
    // because we set hwparam IC_AVOID_RX_FIFO_FLUSH_ON_TX_ABRT to 0.
    if (abort) return PICO_ERROR_GENERIC;
  }

  // do not send "Restart"
  i2c->restart_on_next = false;
  // code originally from pico-sdk until here

  return len;
}

uint16_t eeprom_write_multi(uint8_t reg, uint8_t *src, uint16_t len) {
  const uint8_t page_size = 8;
  const uint8_t buf_size = page_size + 1;
  const uint8_t addr_len = 1;
  uint8_t page_write_buf[buf_size];
  uint16_t remaining = len;
  while (remaining > 0) {
    page_write_buf[0] = reg;
    uint16_t to_write_page = remaining > page_size ? page_size : remaining;
    memcpy(page_write_buf + addr_len, src + (len - remaining), to_write_page);
    i2c_write_blocking(PERSISTENCE_I2C, PERSISTENCE_EEPROM_ADDR, page_write_buf,
                       addr_len, true);
    i2c_write_nors_blocking(page_write_buf + addr_len, to_write_page);
    eeprom_polling();
    remaining -= to_write_page;
    reg += page_size;
  }
  return len;
}

uint16_t eeprom_read(uint8_t reg, uint8_t *dst, uint16_t len) {
  uint8_t buf[] = {reg};
  uint16_t ret;
  i2c_write_blocking(PERSISTENCE_I2C, PERSISTENCE_EEPROM_ADDR, buf, 1, true);
  ret = i2c_read_blocking(PERSISTENCE_I2C, PERSISTENCE_EEPROM_ADDR, dst, len,
                          false);
  return ret;
}

void init_persistence() {
  if (!initialized) {
    initialized = true;

    i2c_init(PERSISTENCE_I2C, 100 * 1000);

    gpio_set_function(PERSISTENCE_EEPROM_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PERSISTENCE_EEPROM_SCL_PIN, GPIO_FUNC_I2C);
  }
}

settings_t read_settings_internal() {
  static settings_t settings = default_settings;
  eeprom_read(PERSISTENCE_EEPROM_START_REG, (uint8_t *)&settings,
              sizeof(settings));
  return settings;
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

int write_settings_to_persistence(settings_t settings) {
  return (int)eeprom_write_multi(PERSISTENCE_EEPROM_START_REG,
                                 (uint8_t *)&settings, sizeof(settings));
}
