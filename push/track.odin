package push

import "midi"
import "core:fmt"
import "core:mem"

Note :: struct {
     channel: midi_t,
       pitch: midi_t,
    velocity: midi_t,
      octave: midi_t,
}

Ghost :: struct {
     index: midi_t,
    octave: midi_t,
}

Knob :: struct {
    index: midi_t,
    value: midi_t,
}

Rect :: struct {
    x: midi_t,
    y: midi_t,
    w: midi_t,
    h: midi_t,
}

Pad :: struct {
    index: midi_t,
       n0: midi_t,
    color: midi_t,
     mode: midi_t,
}

Grid :: struct {
      pads: [64]Pad,
      rect: Rect,
    layout: Layout,
}

Track :: struct {
      index: midi_t,
     octave: midi_t,
       hold: bool,
      strip: int,
       grid: Grid,
     active: [dynamic]midi_t,
       held: [dynamic]midi_t,
     ghosts: [dynamic]Ghost,
    ccknobs: [8]Knob,
}

display_track_grid :: proc(device: ^Device, track: ^Track, r: Rect = { 0, 0, 8, 8 }) {
    using track.grid;
    mem.zero_slice(pads[:]);
    rect = r;
    n, m, i: midi_t;
    for y := r.y; y < (r.y+r.h); y += 1 {
        for x := r.x; x < (r.x+r.w); x += 1 {
            pad: Pad;
            pad.index = y*8+x;
            pad.n0 = n;
            pad.color = layout.colors[n%12];
            pads[m] = pad;
            n += 1;
            m += 1;
            i += 1;
        }
        if r.w > 5 do n -= r.w-5;
    }
    for p,m in pads {
        set_pad(device, p.index, Color(p.color), Mode(p.mode));
    }
}

@(private)
toggle_hold :: proc(device: ^Device, track: ^Track) {

}

@(private)
switch_strip :: proc(device: ^Device, track: ^Track) {

}

@(private)
lookup_pad_from_index :: proc(using track: ^Track, p0: midi_t) -> Pad {
    for pad in grid.pads {
        if pad.index == p0 do return pad;
    }
    unreachable();
}

@(private)
lookup_pad_from_n0 :: proc(using track: ^Track, n0: midi_t) -> Pad {
    for pad in grid.pads {
        if pad.n0 == n0 do return pad;
    }
    unreachable();
}

@(private)
update_octave :: proc(device: ^Device, using track: ^Track, d: i8, event: ^midi.Event) {
    next := i8(octave) + d;
    if next < 0 || next > 10 do return;
    for g in ghosts  do update_note(device, track, g.index, d, 0, event);
    for h in held    do update_note(device, track, h, d, 10, event);
    for a in active  do update_note(device, track, a, d, 0, event);
    clear(&active);
    octave = u8(next);
}

@(private)
update_note :: proc(
    device: ^Device,
    using track: ^Track,
      note: midi_t,
         d: i8,
      mode: midi_t,
     event: ^midi.Event,
){
    n0 := note - octave * 12;
    // these are the min/max n0 values
    // contained in the current grid
    nmin := grid.pads[0].n0;
    nmax := grid.pads[len(grid.pads)-1].n0;
    if n0 >= nmin && n0 <= nmax {
        // turn off out of bounds pads
        pad := lookup_pad_from_n0(track, n0);
        set_grid_pad(device, track, n0, pad.color, 0, event);
    }
    n1 := i8(note) - (i8(octave)+d) * 12;
    // if new pad is out of the grid's bounds
    // do nothing
    if n1 < i8(nmin) || n1 > i8(nmax) do return;
    pad := lookup_pad_from_n0(track, n0);
    set_grid_pad(device, track, u8(n1), grid.layout.pressed, mode, event);
}

@(private)
index_of ::  proc(container: ^$C/[dynamic]$T, data: T) -> int {
    index := -1;
    for n,i in container {
        if n == data {
            index = i;
            break;
        }
    }
    return index;
}


@(private)
contains ::  proc(container: ^$C/[dynamic]$T, data: T) -> bool {
    return index_of(container, data) >= 0;
}

@(private)
is_held ::  proc(using track: ^Track, note: midi_t) -> bool {
    return contains(&held, note);
}

@(private)
is_active ::  proc(using track: ^Track, note: midi_t) -> bool {
    return contains(&active, note);
}

@(private)
remove ::  proc(container: ^$C/[dynamic]$T, data: T) {
    index := -1;
    for n,i in container {
        if n == data {
           index = i;
           break;
        }
    }
    if index >= 0 do ordered_remove(container, index);
}

@(private)
octave_offset ::  proc(using track: ^Track, note: midi_t) -> midi_t {
    return octave*12 + note;
}

@(private)
set_grid_pad :: proc(
    device: ^Device,
    using track: ^Track,
        n0: midi_t,
     color: midi_t,
      mode: midi_t,
     event: ^midi.Event,
){
    n := 0;
    for pad in grid.pads {
        if pad.n0 == n0 {
            event.data[0] = 0x90+mode;
            event.data[1] = pad.index+36;
            event.data[2] = color;
            write_dev_sync(device, event);
            if n == 1 do return;
            n += 1;
        }
    }
}

@(private)
process_device_note_on :: proc(device: ^Device, using track: ^Track, event: ^midi.Event) {
    pad := lookup_pad_from_index(track, midi.index(event)-36);
    n1  := octave_offset(track, pad.n0);
    if is_held(track, n1) do return;
    if is_active(track, n1) {
        // this may happen because of mirror pads
        // in which case, retrigger
        channel := midi.status(event)-0x90;
        event.data[0] = 0x80+channel;
        write_aux_sync(device, event);
        event.data[0] = 0x90+channel;
    }
    event.data[1] = n1;
    write_aux_sync(device, event);
    append(&active, n1);
    set_grid_pad(device, track, pad.n0, grid.layout.pressed, 0, event);
}

@(private)
process_device_note_off :: proc(device: ^Device, using track: ^Track, event: ^midi.Event) {
    pad  := lookup_pad_from_index(track, midi.index(event)-36);
    n1   := octave_offset(track, pad.n0);
    final := pad.n0;
    if track.hold {
        append(&held, n1);
        set_grid_pad(device, track, n1, pad.color, 10, event);
    }
    if is_active(track, n1) do remove(&active, n1);
    else {
        n2 := pad.n0;
        rm := -1;
        for ghost, g in ghosts {
            gn := n2 + ghost.octave * 12;
            if gn == ghost.index {
                final = ghost.index - octave * 12;
                n1 = gn;
                rm = g;
                break;
            }
        }
        if rm == -1 do return;
        else do ordered_remove(&ghosts, rm);
    }
    event.data[1] = n1;
    write_aux_sync(device, event);
    set_grid_pad(device, track, final, pad.color, 0, event);
}

@(private)
on_aftertouch :: proc(device: ^Device, track: ^Track, event: ^midi.Event) {

}
