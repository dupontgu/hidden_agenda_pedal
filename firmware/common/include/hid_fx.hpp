#include "custom_hid.hpp"
#include "hid_output.hpp"

#ifndef HID_FX
#define HID_FX

class IFx {
 public:
  explicit IFx(IHIDOutput *hid_output) { this->hid_output = hid_output; }
  virtual void initialize(uint32_t time_ms, float param_percentage);
  virtual void deinit();
  virtual uint32_t get_current_pixel_value(uint32_t time_ms);
  virtual void update_parameter(float percentage);
  virtual void tick(uint32_t time_ms);

  uint32_t get_indicator_color() { return indicator_color; }

  void set_indicator_color(uint32_t c) { indicator_color = c; }

 protected:
  uint32_t indicator_color = 0xFF666666;
  IHIDOutput *hid_output;
};

class IMouseFx : public IFx {
  using IFx::IFx;

 public:
  virtual void process_mouse_report(ha_mouse_report_t const *report,
                                    uint32_t time_ms) = 0;
  virtual ~IMouseFx() {}
};

class IKeyboardFx : public IFx {
  using IFx::IFx;

 public:
  virtual void process_keyboard_report(ha_keyboard_report_t const *report,
                                       uint32_t time_ms) = 0;
  virtual ~IKeyboardFx() {}
};

#endif