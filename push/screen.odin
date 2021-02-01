package push

import "midi"
import "core:mem"

clear_screen :: proc(using device: ^Device) {
    for n in 0..3 do
        clear_line(device, midi_t(n));
}

clear_line :: proc(using device: ^Device, line: midi_t) {
    index: midi_t = line+0x1c;
    sysx: [8]midi_t = { 0xf0, 0x47, 0x7f, 0x15, index, 0x0, 0x0, 0xf7 };
    write_dev_async(device, sysx[:]);
}

display_text :: proc(using device: ^Device, row: midi_t, col: midi_t, txt: string) {
    sysx: [128]midi_t;
    count := len(txt)+1;
    mem.zero_slice(sysx[:]);
    sysx[0] = 0xf0;
    sysx[1] = 0x47;
    sysx[2] = 0x7f;
    sysx[3] = 0x15;
    sysx[4] = 0x18+row;
    sysx[6] = midi_t(count);
    sysx[7] = col;
    copy(sysx[8:], txt);
    sysx[8+count] = 0xf7;
    write_dev_async(device, sysx[:count+9]);
}
