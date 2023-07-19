#include <stdio.h>
#include <string.h>
#include "test_persistence.hpp"
#include "test_util.hpp"
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
    std::cerr << "FAILURE: " << msg << std::endl;
    dump_logs();
    exit(0);
}

void test_repl() {
    std::cout << "start test_repl..." << std::endl;
    InMemoryPersistence p;
    Repl repl(&p);

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
    assert("raw logging should initially be off", !p.getRawHidLogsEnabled());
    repl.process(input("cmd:raw_hid:on"));
    assert("raw logging should be enabled", p.getRawHidLogsEnabled());
    repl.process(input("cmd:raw_hid:off"));
    assert("raw logging should be disabled", !p.getRawHidLogsEnabled());\

    // COLORS
    assert("second led color should initially be 0", p.getLedColor(1) == 0);
    repl.process(input("cmd:set_color:33:ffeeff"));
    assert("second led color should still be 0", p.getLedColor(1) == 0);
    repl.process(input("cmd:set_color:2:0xffeeff"));
    assert("second led color should be 0xffeeff", p.getLedColor(1) == 0xffeeff);
    repl.process(input("cmd:set_color:3:ffee"));
    assert("third  led color should be 0x00ffee", p.getLedColor(2) == 0x00ffee);

    std::cout << "test_repl pass! ";
    dump_logs();
    reset();
}

int main(int argc, char const *argv[]){
    test_repl();
    return 0;
}
