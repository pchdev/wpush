package push

import "jack"
import "midi"
import "core:time"
import "core:fmt"
import "core:mem"
import "core:c"

midi_t :: u8;

MIDI_BUFFER_SIZE :: 8192;
PUSH_JACK_NAME :: "Ableton Push";

// callbacks
connect_callback :: proc(device: ^Device, udata: rawptr);
disconnect_callback :: proc(device: ^Device, udata: rawptr);

Device :: struct {
         tracks: [dynamic]Track,
      selection: [dynamic]^Track,
        async_dev: midi.Buffer,
        async_aux: midi.Buffer,
            aux: cstring,
        running: bool,
         client: jack.client_t,
    from_device: jack.port_t,
      to_device: jack.port_t,
       from_aux: jack.port_t,
         to_aux: jack.port_t,
     connect_cb: connect_callback,
          udata: rawptr,
}

// initializes jack backend, opening client, creating all the ports
// allocates midi buffers and internal data structures
initialize :: proc(using device: ^Device) {
    client       = jack.open_client("oak-push");
    from_device  = jack.register_port(client, "from_dev", jack.DEFAULT_MIDI_TYPE, .Input);
    to_device    = jack.register_port(client, "to_dev", jack.DEFAULT_MIDI_TYPE, .Output);
    from_aux     = jack.register_port(client, "from_aux", jack.DEFAULT_MIDI_TYPE, .Input);
    to_aux       = jack.register_port(client, "to_aux", jack.DEFAULT_MIDI_TYPE, .Output);
    jack.set_process_callback(client, process_callback, device);
    jack.set_port_connect_callback(client, jack_connect_callback, device);
    async_dev = midi.create_buffer(MIDI_BUFFER_SIZE);
    async_aux = midi.create_buffer(MIDI_BUFFER_SIZE);
    reserve(&tracks, 8);
    reserve(&selection, 4);
}

// stops and closes jack client, deallocates track data
deinitialize :: proc(using device: ^Device) {
    jack.deactivate(client);
    jack.close_client(client);
    delete(selection);
    for _,n in tracks do delete_track(device, &tracks[n]);
    delete(tracks);
}

// ------------------------------------------------------------------------------------------------
// jack/callbacks
// ------------------------------------------------------------------------------------------------

// procedure used to notify whenever push or aux devices change their
// connection status
@(private)
jack_connect_callback :: proc(
    a: jack.port_id_t,
    b: jack.port_id_t,
    connected: c.int,
        udata: rawptr,
){
    device := cast(^Device)(udata);
    ap := jack.port_by_id(device.client, a);
    bp := jack.port_by_id(device.client, b);
    if bool(connected) &&
        ap == device.from_device || bp == device.from_device {
        device.connect_cb(device, device.udata);
    }
}

// registers procedure which will be called whenever push device is online
set_connect_callback ::  proc(
      device: ^Device,
    callback: connect_callback,
       udata: rawptr = nil,
){
    device.connect_cb = callback;
    device.udata = udata;
}

// attempts to connect to both push and aux devices
// running the main process callback (see below)
connect_run :: proc(using device: ^Device, aux_dev: cstring) {
    aux = aux_dev;
    fmt.println("[push] querying jack server for target devices...");
    dev_in  := jack.get_ports(client, PUSH_JACK_NAME, jack.DEFAULT_MIDI_TYPE, .Input);
    dev_out := jack.get_ports(client, PUSH_JACK_NAME, jack.DEFAULT_MIDI_TYPE, .Output);
    aux_in  := jack.get_ports(client, aux_dev, jack.DEFAULT_MIDI_TYPE, .Input);
    aux_out := jack.get_ports(client, aux_dev, jack.DEFAULT_MIDI_TYPE, .Output);
    jack.activate(client);
    if dev_in != nil && dev_out != nil {
        fmt.println("[push] push device online, connecting...");
        jack.connect(client, jack.port_name(to_device), mem.ptr_offset(dev_in, 1)^);
        jack.connect(client, mem.ptr_offset(dev_out, 1)^, jack.port_name(from_device));
    } else {
        fmt.println("[push] push device offline");
    }
    if aux_dev != nil && aux_in != nil && aux_out != nil {
        fmt.println("[push] aux device online, connecting...");
        jack.connect(client, jack.port_name(to_aux), mem.ptr_offset(aux_in, 0)^);
        jack.connect(client, mem.ptr_offset(aux_out, 0)^, jack.port_name(from_aux));
    } else {
        if aux_dev != nil do fmt.println("[push] aux device offline");
    }
    fmt.println("[push] application now running...");
    running = true;
}

@(private)
process_callback :: proc(nframes: jack.nframes_t, udata: rawptr) -> c.int {
    using device := cast(^Device)(udata);
    jmdev: jack.midi_event_t;
    event: midi.Event;
    dev_in  := jack.get_port_buffer(from_device, nframes);
    aux_in  := jack.get_port_buffer(from_aux, nframes);
    dev_out := jack.get_port_buffer(to_device, nframes);
    aux_out := jack.get_port_buffer(to_aux, nframes);
    jack.clear_midi_buffer(dev_out);
    jack.clear_midi_buffer(aux_out);
    nevents := jack.midi_event_count(dev_in);
    // output asynchronous events
    for {
        async := midi.next_event(&async_dev);
        if async == nil do break;
        mdt := jack.reserve_midi_event(dev_out, 0, auto_cast(len(async.data)));
        mem.copy(mdt, raw_data(async.data), auto_cast(len(async.data)));
    }
    // process incoming device events (todo with aux)
    for n in 0..<nevents {
        jack.get_midi_event(&jmdev, dev_in, u32(n));
        event.frame = uint(jmdev.time);
        event.data  = mem.slice_ptr(cast(^midi_t)(jmdev.buffer), int(jmdev.size));
        process_device_event(device, &event);
    }
    midi.zero_buffer(&async_dev);
    midi.zero_buffer(&async_aux);
    return 0;
}

// ------------------------------------------------------------------------------------------------
// device/incoming
// ------------------------------------------------------------------------------------------------

// called whenever push device is sending input data, redirecting to the proper
// event procedures below
@(private)
process_device_event :: proc(using device: ^Device, event: ^midi.Event) {
    track: ^Track = selection[0];
    switch (midi.status(event)) {
    case 0x80:
        if midi.index(event) >= 36 {
            process_device_note_off(device, track, event);
        }
    case 0x90:
        if midi.index(event) >= 36 {
            process_device_note_on(device, track, event);
        }
//        else do switch(Sensor(midi.index(event))) {
//            case .Swing:
//                switch_strip(device, track);
    case 0xa0:
        on_aftertouch(device, track, event);
    case 0xb0:
        switch (midi.index(event)) {
        case 1, 7, 10:
            // write_sync_aux()
        case 20..27, 102..109:
            process_toggle(device, event);
        case 71..79:
            process_knob(device, event);
        case:
            process_button(device, event);
        }
    case 0xe0:
    }
}

@(private)
process_button :: proc(using device: ^Device, event: ^midi.Event) {

}

@(private)
process_toggle :: proc(using device: ^Device, event: ^midi.Event) {

}

@(private)
process_knob :: proc(using device: ^Device, event: ^midi.Event) {

}

// ------------------------------------------------------------------------------------------------
// device/writing
// ------------------------------------------------------------------------------------------------

@(private)
write_aux_sync :: proc(using device: ^Device, event: ^midi.Event) {
    nbytes := len(event.data);
    buffer := jack.get_port_buffer(to_aux, 512);
    mdt := jack.reserve_midi_event(buffer, 0, auto_cast(nbytes));
    mem.copy(mdt, raw_data(event.data), nbytes);
    fmt.println("[push] sending data to auxiliary device (sync):", event.data);
}

@(private)
write_dev_sync ::  proc(using device: ^Device, event: ^midi.Event) {
    nbytes := len(event.data);
    buffer := jack.get_port_buffer(to_device, 512);
    mdt := jack.reserve_midi_event(buffer, 0, auto_cast(nbytes));
    mem.copy(mdt, raw_data(event.data), nbytes);
    fmt.println("[push] sending data to physical device (sync):", event.data);
}

@(private)
write_dev_async ::  proc(using device: ^Device, data: []midi_t) {
    nbytes := len(data);
    ev := midi.pull_event(&async_dev, auto_cast(nbytes));
    mem.copy(raw_data(ev.data), raw_data(data), nbytes);
    fmt.println("[push] sending data to physical device (async):", data);
}

// ------------------------------------------------------------------------------------------------
// asynchronous functions
// ------------------------------------------------------------------------------------------------

// turns on/off a pad on the physical device's grid
set_pad ::  proc(using device: ^Device, index: midi_t, color: Color, mode: Mode = .Off) {
    mdt := [3]midi_t { midi_t(mode)+0x90, index+36, midi_t(color) };
    write_dev_async(device, mdt[:]);
}

// turns on/off a toggle pad on the physical device's upper/lower rows
set_toggle ::  proc(using device: ^Device, row: Toggle_Row, index: midi_t, mode: Mode) {
    mdt := [3]midi_t { 0xb0, midi_t(row)+index, midi_t(mode) };
    write_dev_async(device, mdt[:]);
}

// turns on/off a specific device button
set_button ::  proc(using device: ^Device, index: Button, mode: Mode) {
    mdt := [3]midi_t { 0xb0, midi_t(index), midi_t(mode) };
    write_dev_async(device, mdt[:]);
}

// sets device's left-side strip/ribbon's mode
set_strip_mode ::  proc(using device: ^Device, mode: Strip) {
    sysx := [9]midi_t { 0xf0, 0x47, 0x7f, 0x15, 0x63, 0x0, 0x1, midi_t(mode), 0xf7 };
    write_dev_async(device, sysx[:]);
}

// ------------------------------------------------------------------------------------------------
// internal
// ------------------------------------------------------------------------------------------------

create_track :: proc(using device: ^Device, l: Layout = LAYOUT_DEFAULT) {
    fmt.printf("[push] creating track n°%d\n", len(tracks)+1);
    index := len(tracks);
    append(&tracks, Track {
          index = u8(index),
         octave = 3,
           hold = false,
          strip = 0,
           grid = Grid { pads = {}, rect = {}, layout = l },
         active = make([dynamic]midi_t, 0, 16),
           held = make([dynamic]midi_t, 0, 16),
         ghosts = make([dynamic]Ghost, 0, 16),
        ccknobs = {},
    });
    track, ok := get_track(device, index);
    assert(ok);
    set_toggle(device, .Upper, midi_t(index), .Yellow_Dim);
}

delete_track :: proc(using device: ^Device, track: ^Track) {
    delete(track.active);
    delete(track.held);
    delete(track.ghosts);
}

@(require_results)
get_track :: proc(using device: ^Device, index: int) -> (^Track, bool) {
    if len(tracks) <= index do return nil, false;
    else do return &tracks[index], true;
}

select_track :: proc(using device: ^Device, track: ^Track) {
    // add to selected tracks array
    // set toggle
    append(&selection, track);
    set_toggle(device, .Upper, track.index, .Green_Full);
}
