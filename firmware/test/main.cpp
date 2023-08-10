#include <stdio.h>
#include <string.h>
#include "test_persistence.hpp"
#include "test_util.hpp"
#include "test_hid_output.hpp"
#include "repl.hpp"

char in_buf[512] = { 0 };

char *input(std::string s) {
    memcpy(in_buf, s.c_str(), s.size());
    in_buf[s.size()] = '\n';
    in_buf[s.size() + 1] = 0;
    return in_buf;
}

void assert(std::string msg, bool b) {
    if (b) return;
    dump_logs();
    std::cerr << "FAILURE: " << msg << std::endl;
    exit(0);
}

void test_repl() {
    std::cout << "start test_repl..." << std::endl;
    InMemoryPersistence p;
    TestHIDOutput hid;
    p.initialize();
    Repl repl(&p, &hid);

    // JUNK
    repl.process(input("junk1234"));

    // BRIGHTNESS
    assert("initial brightness should be 0", p.getLedBrightness() < 0.000001f);
    repl.process(input("cmd:brightness:99"));
    assert("brightness should be 0.99", p.getLedBrightness() == 0.99f);

    repl.process(input("cmd:brightness"));
    assert("brightness should be 0.99", p.getLedBrightness() == 0.99f);

    repl.process(input("cmd:brightness:"));
    assert("brightness should be 0.99", p.getLedBrightness() == 0.99f);

    repl.process(input("cmd:brightness:101"));
    assert("brightness should be 0.99", p.getLedBrightness() == 0.99f);

    repl.process(input("cmd:brightness:0"));
    assert("brightness should be 0.99", p.getLedBrightness() == 0.99f);

    repl.process(input("cmd:brightness:40"));
    assert("brightness should be 0.4", p.getLedBrightness() == 0.4f);
    
    //REBOOT
    assert("reboot should not have been hit yet", reboot_count == 0);
    repl.process(input("cmd:boot"));
    assert("reboot should have been hit once", reboot_count == 1);

    //LOGS
    assert("raw logging should initially be off", !p.areRawHidLogsEnabled());
    repl.process(input("cmd:raw_hid:on"));
    assert("raw logging should be enabled", p.areRawHidLogsEnabled());
    repl.process(input("cmd:raw_hid:off"));
    assert("raw logging should be disabled", !p.areRawHidLogsEnabled());

    //FOOTSWITCH
    assert("footswitch inversion should initially be off", !p.shouldInvertFootswitch());
    repl.process(input("cmd:invert_foot:on"));
    assert("footswitch inversion should be enabled", p.shouldInvertFootswitch());
    repl.process(input("cmd:invert_foot:off"));
    assert("footswitch inversion should be disabled", !p.shouldInvertFootswitch());

    //LED FLASHING
    assert("flashing should initially be on", p.isFlashingEnabled());
    repl.process(input("cmd:flash:off"));
    assert("flashing should be disabled", !p.isFlashingEnabled());
    repl.process(input("cmd:flash:on"));
    assert("flashing should be enabled", p.isFlashingEnabled());

    //COLORS
    assert("second led color should initially be 0", p.getLedColor(1) == 0);
    repl.process(input("cmd:set_color:33:ffeeff"));
    assert("second led color should still be 0", p.getLedColor(1) == 0);
    repl.process(input("cmd:set_color:2:0xffeeff"));
    assert("second led color should be 0xffeeff", p.getLedColor(1) == 0xffeeff);
    repl.process(input("cmd:set_color:3:ffee"));
    assert("third led color should be 0x00ffee", p.getLedColor(2) == 0x00ffee);

    //MOUSE SPEED
    assert("mouse speed level should initially be 0", p.getMouseSpeedLevel() == 0);
    repl.process(input("cmd:m_speed:100"));
    assert("mouse speed level should still be 0", p.getMouseSpeedLevel() == 0);
    repl.process(input("cmd:m_speed:3"));
    assert("mouse speed level should be 2", p.getMouseSpeedLevel() == 2);

    //RESET DEFAULTS
    repl.process(input("cmd:reset"));
    assert("third led color should be reset to 0", p.getLedColor(2) == 0);
    assert("mouse speed should be reset to 0", p.getMouseSpeedLevel() == 0);
    assert("brightness should be reset to 0", p.getLedBrightness() == 0.0f);

    dump_logs();
    std::cout << "test_repl PASS!" << std::endl;
    reset();
}

int main(int argc, char const *argv[]){
    test_repl();
    return 0;
}
