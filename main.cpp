#include <iostream>
#include "wpush.hpp"
#include "enums.hpp"
#include <chrono>
#include <thread>

using namespace wpn214::push;

int main()
{
    scheme s = { 0, 101, 86, 16 };
    auto chroma = layout::chromatic(s);
    wpn214::push::device dev;
    auto& t1 = dev.create_track(chroma);

    try {
        auto& backend = dev.set_backend(backend::jack);
        backend.set_connected_cb([&] {
            t1.select();
            t1.display({8, 8, 0, 0});
            dev.screen_clear();
            dev.screen_display(0, 31, "WPN214");
            dev.set_button(button::OctaveUp, button::mode::Full);
            dev.set_button(button::OctaveDown, button::mode::Full);
            dev.set_button(button::Select, button::mode::Full);
            dev.set_strip(strip::Pitchbend);
        });
        dev.connect("REAPER");
    } catch (std::exception& e) {
        printf("error: could not set specified backend (%s), "
               "aborting\n", e.what());
        exit(EXIT_FAILURE);
    }
    while (1) {
        std::this_thread::sleep_for(
             std::chrono::milliseconds(1000));
    }
    return 0;
}
