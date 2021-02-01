package main

import "push"
import "core:time"
import "core:fmt"

seconds :: inline proc(n: time.Duration) -> time.Duration {
    return n * time.Second;
}

on_connected :: proc(device: ^push.Device, udata: rawptr) {
    push.clear_screen(device);
    push.display_text(device, 0, 31, "WPN214");
    track, ok := push.get_track(device, 0);
    if ok {
        push.select_track(device, track);
        push.display_track_grid(device, track);
    } else {
        fmt.println("could not select first track");
        unreachable();
    }
}

main :: proc() {
    device: push.Device;
    push.initialize(&device);
    defer push.deinitialize(&device);
    push.set_connect_callback(&device, on_connected);
    push.create_track(&device);
    push.connect_run(&device, "REAPER");
    for {
        if device.running do
           time.sleep(seconds(1));
        else do break;
    }
}
