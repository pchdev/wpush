#pragma once
#include "wmidi.hpp"
#include <vector>
#include <functional>
#include <string>

namespace wpn214 {
namespace push {

struct rect {
    midi_t w;
    midi_t h;
    midi_t x;
    midi_t y;
};

struct scheme {
    midi_t dark;
    midi_t medium;
    midi_t bright;
    midi_t pressed;
};

class layout {
public:
    static layout chromatic(scheme s) {
        layout l;
        l.pressed = s.pressed;
        l.colors[0] = s.bright;
        l.colors[1] = s.dark;
        l.colors[2] = s.medium;
        l.colors[3] = s.dark;
        l.colors[4] = s.medium;
        l.colors[5] = s.medium;
        l.colors[6] = s.dark;
        l.colors[7] = s.medium;
        l.colors[8] = s.dark;
        l.colors[9] = s.medium;
        l.colors[10] = s.dark;
        l.colors[11] = s.medium;
        return l;
    }
    midi_t pressed;
    midi_t colors[12];
};

struct note {
    midi_t channel;
    midi_t pitch;
    midi_t velocity;
    midi_t octave;
};

class device;

class grid {
public:
    struct pad {
        midi_t index;
        midi_t n0;
        midi_t color;
        midi_t mode;
    };

public:
    grid() { m_pads.reserve(64); }
    void set_layout(layout const& l) { m_layout = l; }
    wpn214::push::layout& layout() { return m_layout; };

    void set_rect(rect const r) {
        m_pads.clear();
        midi_t x, y, n = 0, i = 0;
        for (y = r.y; y < (r.y+r.h); ++y) {
            for (x = r.x; x < (r.x+r.w); ++x) {
                grid::pad pad;
                pad.index = y*8+x;
                pad.n0 = n;
                pad.color = m_layout.colors[n%12];
                pad.mode = 0;
                m_pads.push_back(pad);
                n++; i++;
            }
            if (r.w > 5)
                n -= r.w-5;
        }
        m_rect = r;
    }

    grid::pad& lookup_index(midi_t p0) {
        for (auto& pad : m_pads)
            if (pad.index == p0)
                return pad;
    }

    grid::pad& lookup_n0(midi_t n0) {
        for (auto& pad : m_pads)
            if (pad.n0 == n0)
                return pad;
    }

    void write(midi_t n0, midi_t color, midi_t mode,
               mdev &ev, device& dev);

    std::vector<grid::pad>& pads() { return m_pads; }

private:
    std::vector<grid::pad> m_pads;
    rect m_rect;
    class layout m_layout;
};

class mdwriter {
public:
    using mdw_fn = std::function<void(mdev const&, void*)>;
    mdwriter() { }
    mdwriter(mdw_fn fn) : m_wfunc(fn) {}

    void set_wfn(mdw_fn fn) { m_wfunc = fn; }

    void write(midi_t* data, midi_t nbytes, midi_t frame, void* out) {
        mdev ev = { frame, nbytes, data };
        m_wfunc(ev, out);
    }

    void write(mdev const& ev, void* out) {
        m_wfunc(ev, out);
    }

private:
    mdw_fn m_wfunc;
};

class track {
public:
    struct ghost {
        midi_t index;
        midi_t octave;
        bool operator==(ghost const& rhs) const {
            return index == rhs.index && octave == rhs.octave;
        }
    };

    track(device& dev, midi_t index) :
        m_device(dev), m_index(index) {}

    midi_t index() const noexcept { return m_index; }
    void set_layout(layout const& l);
    void display(rect r, mdwriter* w = nullptr, midi_t frame = 0);
    void select();
    void set_octave(midi_t octave, mdwriter* w = nullptr, midi_t frame = 0);
    void dev_note_on(mdev& ev);
    void dev_note_off(mdev& ev);
    void aftertouch(mdev& ev);

    void update_octave(mdev& ev, int8_t delta, mdwriter* w = nullptr, midi_t frame = 0);

private:
    void update_note(mdev& ev, midi_t note, int8_t delta, midi_t mode,
                     mdwriter* w = nullptr, midi_t frame = 0);

    device& m_device;
    midi_t m_index;
    midi_t m_octave = 3;
    bool m_hold = false;
    bool m_select = false;
    grid m_grid;
    std::vector<midi_t> m_active;
    std::vector<midi_t> m_held;
    std::vector<ghost> m_ghosts;
};

class backend {
public:
    backend(device& dev) : m_dev(dev) {}
    enum { alsa, jack };
    virtual void set_aux(std::string aux) = 0;
    virtual void connect() = 0;
    virtual void start() = 0;
    virtual mdwriter::mdw_fn wsync_fn() const = 0;
    virtual void* dev_out_data() = 0;
    virtual void* aux_out_data() = 0;
    virtual void stop() = 0;
protected:
    device& m_dev;
};

class device {
public:
    // there might be a lot of sysx/standard midi messages
    // sent during initialization, 8192 bytes is probably enough
    device(ushort mdcapacity = 8192) :
        m_async_buffer(mdcapacity) {
        m_wasync = mdwriter([this](mdev const& ev, void* data) {
           // note: we should be able to write aux out events as well..
           m_async_buffer.write(ev);
        });
    }
    class track& operator[](unsigned int index) {
        for (auto& track : m_tracks)
            if (track.index() == index)
                return track;
        assert(false);
    }
    class track& track(unsigned int index) {
        return operator[](index);
    }

    void write_sync_aux(const mdev& event);
    void write_sync_dev(const mdev& event);
    mdbuf& async_buffer() { return m_async_buffer; }
    void set_backend(int type, std::string aux);

    class track& create_track(mdwriter* w = nullptr, midi_t frame = 0);
    class track& create_track(layout l, mdwriter* w = nullptr, midi_t frame = 0);
    void remove_track(mdwriter* w = nullptr, midi_t frame = 0);

    void set_ribbon(mdwriter* w = nullptr, midi_t frame = 0);
    void screen_clearline(midi_t lineno, mdwriter* w = nullptr, midi_t frame = 0);
    void screen_clear(mdwriter* w = nullptr, midi_t frame = 0);
    void screen_display(midi_t row, midi_t col, std::string str,
                        mdwriter* w = nullptr, midi_t frame = 0);

    void set_button(midi_t index, midi_t mode,
                    mdwriter* w = nullptr, midi_t frame = 0);

    void set_toggle(midi_t row, midi_t index, midi_t mode,
                    mdwriter* w = nullptr, midi_t frame = 0);

    void set_pad(midi_t index, midi_t color, midi_t mode,
                 mdwriter* w = nullptr, midi_t frame = 0);

    void process_mdev(mdev& ev);
    void process_tick(ushort nframes);

private:
    void process_toggle(mdev& ev);
    void process_knob(mdev& ev);
    void process_button(mdev& ev);
    void mdwrite(midi_t status, midi_t b1, midi_t b2,
                 mdwriter* w = nullptr, midi_t frame = 0);

    std::vector<class track> m_tracks;
    std::vector<class track*> m_sel;
    class layout m_layouts[4];
    std::unique_ptr<class backend> m_backend;
    class mdbuf m_async_buffer;
    mdwriter m_wsync;
    mdwriter m_wasync;
    void* m_dev_out = nullptr;
    void* m_aux_out = nullptr;
};
}
}
