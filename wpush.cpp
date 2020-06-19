#include "wpush.hpp"
#include "wjack.hpp"
#include <string>
#include <algorithm>
#include "enums.hpp"

using namespace wpn214::push;
using namespace wpn214;

#define _status(_mdev) \
_mdev.data[0]
#define _index(_mdev)  \
_mdev.data[1]
#define _value(_mdev)  \
_mdev.data[2]
#define _release(_mdev)    \
if (_mdev.data[2] == 0)

//-----------------------------------------------------------------------------
// KNOB
//-----------------------------------------------------------------------------

midi_t ccknob::update(midi_t incdec)
{
    if (incdec > 100) {
        m_value -= 127-incdec;
        if (m_value < 0)
            m_value = 0;
    } else {
        m_value += incdec;
        if (m_value > 127)
            m_value = 127;
    }
    return m_value;
}

//-----------------------------------------------------------------------------
// GRID
//-----------------------------------------------------------------------------

void grid::write(midi_t n0, midi_t color, midi_t mode,
                 mdev& ev, device& dev)
{
    midi_t n = 0;
    for (auto& pad : m_pads)
        if (pad.n0 == n0) {
            _status(ev) = 0x90+mode;
            _index(ev) = pad.index+36;
            _value(ev) = color;
            dev.write_sync_dev(ev);
            if (n++ == 1)
                // 2 pads max currently
                // we do not want it to go through the whole array
                return;
        }
}

//-----------------------------------------------------------------------------
// TRACK
//-----------------------------------------------------------------------------

void track::set_layout(const layout &l)
{
    m_grid.set_layout(l);
}

void track::display(rect r, mdev ev)
{
    midi_t npads = r.w*r.h;
    m_grid.set_rect(r);
    for (auto& pad : m_grid.pads())
        m_device.set_pad(pad.index, pad.color, pad.mode, ev);
}

void track::select()
{

}

void track::set_hold(mdev ev)
{
    m_hold = !m_hold;
}

void track::stripswitch(mdev ev)
{
    m_strip = m_strip == strip::Pitchbend ?
              strip::Modwheel : strip::Pitchbend;
    m_device.set_strip(m_strip, ev);
}

void track::update_octave(int8_t d, mdev& ev)
{
    if (m_octave+d < 0 || m_octave+d > 10)
        return;
    for (auto& g : m_ghosts)
        update_note(g.index, d, 0, ev);
    for (auto& h : m_held)
        update_note(h, d, 10, ev);
    for (auto& a : m_active) {
        ghost g = { a, m_octave };
        m_ghosts.push_back(g);
        update_note(a, d, 0, ev);
    }
    m_active.clear();
    m_octave += d;
}

void track::update_note(midi_t note, int8_t d, midi_t mode, mdev& ev)
{
    int8_t n0 = note-m_octave*12;
    // these are the min/max n0 values
    // contained in the current grid
    midi_t nmin = m_grid.pads().front().n0;
    midi_t nmax = m_grid.pads().back().n0;
    if (n0 >= nmin && n0 <= nmax) {
        // turn off out of bounds pads
        const auto& gpad = m_grid.lookup_n0(n0);
        m_grid.write(n0, gpad.color, 0, ev, m_device);
    }
    int8_t n1 = note-(m_octave+d)*12;
    // if new pad is out of the grid's bounds
    // do nothing
    if (n1 < nmin || n1 > nmax)
        return;

    const auto& gpad = m_grid.lookup_n0(n1);
    m_grid.write(n1, m_grid.layout().pressed, mode, ev, m_device);
}

template<typename T> inline
bool contains(std::vector<T> const& vec, T& value)
{
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

template<typename T> inline
void remove(std::vector<T>& vec, T& value)
{
    vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
}

#define _octoffset(_n0) \
_n0+m_octave*12

void track::dev_note_on(mdev& ev)
{
    auto const& gpad = m_grid.lookup_index(_index(ev)-36);
    midi_t nt1 = _octoffset(gpad.n0);

    if (contains(m_held, nt1))
        return;
    if (contains(m_active, nt1)) {
        // this may happen because of mirror pads,
        // in which case, retrigger
        midi_t chn = _status(ev)-0x90;
        _status(ev) = 0x80+chn;
        m_device.write_sync_aux(ev);
        _status(ev) = 0x90+chn;
    }
    _index(ev) = nt1;
    m_device.write_sync_aux(ev);
    m_active.push_back(nt1);
    m_grid.write(gpad.n0, m_grid.layout().pressed,
                 0, ev, m_device);
}

void track::dev_note_off(mdev& ev)
{
    auto const& gpad = m_grid.lookup_index(_index(ev)-36);
    midi_t nt1 = _octoffset(gpad.n0);
    midi_t final = gpad.n0;

    if (m_hold) {
        m_held.push_back(nt1);
        m_grid.write(nt1, gpad.color, 10, ev, m_device);
    }
    if (contains(m_active, nt1))
        remove(m_active, nt1);
    else {
        midi_t nt2 = gpad.n0;
        ghost* rem = nullptr;
        for (auto& ghost : m_ghosts) {
            midi_t gn = nt2+ ghost.octave*12;
            if (gn == ghost.index) {
                final = ghost.index-m_octave*12;
                nt1 = gn;                
                rem = &ghost;
                break;
            }
        }
        if (rem == nullptr)
            return;
        else
            remove<ghost>(m_ghosts, *rem);            
    }
    _index(ev) = nt1;
    m_device.write_sync_aux(ev);
    m_grid.write(final, gpad.color, 0, ev, m_device);
}

void track::aftertouch(mdev& ev)
{
    // we should check active note velocity
    // trigger aftertouch only if it goes beyond a lot
    auto const& gpad = m_grid.lookup_index(_index(ev)-36);
    _status(ev) += m_index;
    _index(ev) = _octoffset(gpad.n0);
    m_device.write_sync_aux(ev);
}

//-----------------------------------------------------------------------------
// DEVICE
//-----------------------------------------------------------------------------

inline void
device::write_sync_aux(mdev const& event)
{
    m_wsync.write(event, m_aux_out);
}

inline void
device::write_sync_dev(mdev const& event)
{
    m_wsync.write(event, m_dev_out);
}

inline void
device::mdwrite(midi_t arr[], midi_t size, mdev ev, void* output)
{
    bool null = ev.null();
    ev.data = arr;
    ev.nbytes = size;
    if (null)
        m_wasync.write(ev, output);
    else
        m_wsync.write(ev, output);
}

backend& device::set_backend(int type)
{
    switch (type) {
    case backend::alsa:
        throw std::exception();
        break;
    case backend::jack: {
        m_backend = std::unique_ptr<jack_backend>(
                    new jack_backend(*this));
        break;
    }
    }
    m_wsync.set_wfn(m_backend->wsync_fn());
    m_dev_out = m_backend->dev_out_data();
    m_aux_out = m_backend->aux_out_data();
    return *m_backend;
}

void device::connect(std::string aux)
{
    m_backend->set_aux(aux);
    m_backend->start();
}

track&
device::create_track(mdev ev)
{
    m_tracks.emplace_back(*this, m_tracks.size());
    auto& t = m_tracks.back();
    t.set_layout(m_layouts[m_tracks.size()%4]);
    return t;
}

track&
device::create_track(layout l, mdev ev)
{
    auto& t = create_track(ev);
    t.set_layout(l);
    return t;
}

void
device::set_strip(midi_t mode, mdev ev)
{
    midi_t sysx[9] = { 0xf0, 0x47, 0x7f, 0x15, 0x63, 0x0, 0x1, mode, 0xf7 };
    mdwrite(sysx, sizeof(sysx), ev, m_dev_out);
}

void
device::screen_clearline(midi_t lineno, mdev ev)
{
    midi_t index = lineno+0x1c;
    midi_t sysx[] = { 0xf0, 0x47, 0x7f, 0x15, index, 0x0, 0x0, 0xf7 };
    mdwrite(sysx, sizeof(sysx), ev, m_dev_out);
}

void
device::screen_clear(mdev ev)
{
    for (int n = 0; n < 4; ++n)
        screen_clearline(n, ev);
}

void
device::screen_display(midi_t row, midi_t col,
                       std::string str, mdev ev)
{
    midi_t sysx[128];
    midi_t len = str.size()+1;
    memset(sysx, 0, sizeof(sysx));
    sysx[0] = 0xf0;
    sysx[1] = 0x47;
    sysx[2] = 0x7f;
    sysx[3] = 0x15;
    sysx[4] = 0x18+row;
    sysx[6] = len;
    sysx[7] = col;
    memcpy(&sysx[8], str.c_str(), len);
    sysx[8+len] = 0xf7;
    mdwrite(sysx, len+9, ev, m_dev_out);
}

inline void
device::mdwrite(midi_t status, midi_t b1, midi_t b2, mdev ev)
{
    midi_t mdt[] = { status, b1, b2 };
    mdwrite(mdt, sizeof(mdt), ev, m_dev_out);
}

void
device::set_button(midi_t index, midi_t mode, mdev ev)
{
    mdwrite(0xb0, index, mode, ev);
}

void
device::set_toggle(midi_t row, midi_t index, midi_t mode, mdev ev)
{
    mdwrite(0xb0, row+index, mode, ev);
}

void
device::set_pad(midi_t index, midi_t color, midi_t mode, mdev ev)
{
    mdwrite(0x90+mode, index+36, color, ev);
}

void device::process_knob(mdev& ev)
{
    char vstr[4] = {0};
    auto& track = m_tracks[0];
    midi_t index = _index(ev)-71;
    auto value = track.knob(index).update(_value(ev));
    sprintf(vstr, "%d", value);
    screen_display(1, index*8-(strlen(vstr)/2), vstr, ev);
}

void device::process_toggle(mdev& ev)
{

}

void device::process_button(mdev& ev)
{
    auto& track = m_tracks[0];

    switch (_index(ev)) {
    case button::OctaveUp: {
        _release(ev)
            track.update_octave(1, ev);
        break;
    }
    case button::OctaveDown: {
        _release(ev)
            track.update_octave(-1, ev);
        break;
    }
    case button::Select: {
        _release(ev)
            track.set_hold(ev);
        break;
    }
    }
}

void device::process_mdev(mdev& ev)
{
    class track& wt = m_tracks[0];

    switch (_status(ev) & 0xf0) {
    case 0x80: {
        if (_index(ev) >= 36)
            // otherwise they are emitted by
            // the knob/ribbon sensors etc.
            wt.dev_note_off(ev);
        break;
    }
    case 0x90: {
        if (_index(ev) >= 36)
            wt.dev_note_on(ev);
        else switch (_index(ev)) {
        case sensors::Swing:
            wt.stripswitch(ev);
            break;
        }
        break;
    }
    case 0xa0: { // aftertouch
        wt.aftertouch(ev);
        break;
    }
    case 0xb0: { // control
        switch(_index(ev)) {
        case 1: // modwheel (strip)
        case 7: // volume (strip)
        case 10: // pan (strip)
            write_sync_aux(ev);
            break;
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
            process_toggle(ev);
            break;
        case 71:
        case 72:
        case 73:
        case 74:
        case 75:
        case 76:
        case 77:
        case 78:
        case 79:
            process_knob(ev);
            break;
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107:
        case 108:
        case 109:
            process_toggle(ev);
            break;
        default:
            process_button(ev);
        }
        break;
    }
    case 0xe0: { // pitchbend
        write_sync_aux(ev);
        break;
    }
    }
}

void device::process_tick(ushort nframes)
{

}
