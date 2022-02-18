package midi

import "core:mem";
import "core:fmt";

midi_t :: u8;

Note :: enum midi_t {
    C0   = 00, CS0, D0, DS0, E0, F0, FS0, G0, GS0, A0, AS0, B0,
    C1   = 12, CS1, D1, DS1, E1, F1, FS1, G1, GS1, A1, AS1, B1,
    C2   = 24, CS2, D2, DS2, E2, F2, FS2, G2, GS2, A2, AS2, B2,
    C3   = 36, CS3, D3, DS3, E3, F3, FS3, G3, GS3, A3, AS3, B3,
    C4   = 48, CS4, D4, DS4, E4, F4, FS4, G4, GS4, A4, AS4, B4,
    C5   = 60, CS5, D5, DS5, E5, F5, FS5, G5, GS5, A5, AS5, B5,
    C6   = 72, CS6, D6, DS6, E6, F6, FS6, G6, GS6, A6, AS6, B6,
    C7   = 84, CS7, D7, DS7, E7, F7, FS7, G7, GS7, A7, AS7, B7,
    C8   = 96, CS8, D8, DS8, E8, F8, FS8, G8, GS8, A8, AS8, B8,
    C9  = 108, CS9, D9, DS9, E9, F9, FS9, G9, GS9, A9, AS9, B9,
    C10 = 120, CS10, D10, DS10, E10, F10, FS10, G10,
}

Control :: enum midi_t {
       Bank_Select = 0,
        Modulation = 1,
 Breath_Controller = 2,
   Foot_Controller = 4,
   Portamento_Time = 5,
}

Status :: enum midi_t {
      Note_off = 0x80,
       Note_on = 0x90,
    Aftertouch = 0xa0,
       Control = 0xb0,
      Pressure = 0xc0,
     Pitchbend = 0xe0,
}

Event :: struct {
     frame: uint,
      data: []midi_t,
}

status :: proc(using event: ^Event) -> midi_t {
    return data[0] & 0xf0;
}

index :: proc(using event: ^Event) -> midi_t {
    return data[1];
}

value :: proc(using event: ^Event) -> midi_t {
    return data[2];
}

is_null :: proc(using event: ^Event) -> bool {
    return len(data) == 0;
}

Buffer :: struct {
    r: uint, w: uint,
    data: []midi_t,
}

create_buffer :: proc(capacity: uint) -> Buffer {
    fmt.printf("[midi] allocating buffer (%d bytes)\n", capacity);
    using buf := Buffer {
        r = 0, w = 0,
        data = make([]midi_t, capacity),
    };
    zero_buffer(&buf);
    return buf;
}

zero_buffer :: proc(using buffer: ^Buffer) {
    mem.zero_slice(data);
}

pull_event :: proc(buffer: ^Buffer, nbytes: uint) -> ^Event {
    w := buffer.w;
    s := uint(size_of(Event));
    nwi: uint = w+s+nbytes;
    if int(nwi) > len(buffer.data) do return nil;
    else {
        event := transmute(^Event)(&buffer.data[w]);
        event.frame = 0;
        event.data = buffer.data[w+s:nwi];
        buffer.w = nwi;
        return event;
    }
}

write_event :: proc(buffer: ^Buffer, event: Event) {
    target := pull_event(buffer, uint(len(event.data)));
    mem.copy(raw_data(target.data), raw_data(event.data), len(event.data));
}

next_event :: proc(buffer: ^Buffer) -> ^Event {
    r := buffer.r; // atomic load
    s := uint(size_of(Event));
    if int(r) == len(buffer.data) || r == buffer.w do return nil;
    else {
        event := transmute(^Event)(&buffer.data[r]);
        r += uint(len(event.data))+s;
        buffer.r = r; // atomic store
        return event;
    }
}

