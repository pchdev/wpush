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
    void set_layout(layout const& l) noexcept { m_layout = l; }
    wpn214::push::layout& layout() noexcept { return m_layout; };

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

    grid::pad const& lookup_index(midi_t p0) const {
        for (auto& pad : m_pads)
            if (pad.index == p0)
                return pad;
        throw std::exception();
    }

    grid::pad const& lookup_n0(midi_t n0) const {
        for (auto& pad : m_pads)
            if (pad.n0 == n0)
                return pad;
        throw std::exception();
    }

    void write(midi_t n0, midi_t color, midi_t mode,
               mdev &ev, device& dev);

    std::vector<grid::pad>& pads() noexcept { return m_pads; }

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

class ccknob {
public:
    ccknob() {}
    ccknob(midi_t index) : m_index(index) {}
    midi_t update(midi_t incdec);
    midi_t value() const { return m_value; }
    midi_t index() const { return m_index; }
    void set_index(midi_t index) { m_index = index; }
private:
    midi_t m_index = 0;
    midi_t m_value = 0;
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
    void display(rect r, mdev ev = {});
    void select();
    void set_hold(mdev ev = {});
    void stripswitch(mdev ev = {});
    void dev_note_on(mdev& ev);
    void dev_note_off(mdev& ev);
    void aftertouch(mdev& ev);
    ccknob& knob(midi_t index) {
        assert(index < m_cc.size());
        return m_cc[index];
    }

    void update_octave(mdev& ev, int8_t delta);

private:
    void update_note(mdev& ev, midi_t note, int8_t delta, midi_t mode);

    device& m_device;
    midi_t m_index;
    midi_t m_octave = 3;
    bool m_hold    = false;
    bool m_select  = false;
    midi_t m_strip = 5;
    grid m_grid;
    std::vector<midi_t> m_active;
    std::vector<midi_t> m_held;
    std::vector<ghost> m_ghosts;
    std::array<ccknob, 8> m_cc;
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
    virtual void set_connected_cb(std::function<void()> cb) {
        m_connected_cb = cb;
    }

protected:
    device& m_dev;
    std::function<void()> m_connected_cb;
};

//class screen {
//public:
//    screen(device& dev, midi_t div);
//    void display(midi_t line, midi_t div, std::string const& str);
//private:
//    byte_t* m_buffer = nullptr;
//    device& m_dev;
//};

class device {
public:
    // there might be a lot of sysx/standard midi messages
    // sent during initialization, 8192 bytes is probably enough
    device(ushort mdcapacity = 8192) :
        m_async_buffer(mdcapacity) {
        m_sel.reserve(8);
        m_wasync = mdwriter([this](mdev const& ev, void* data) {
           // note: we should be able to write aux out events as well..
           m_async_buffer.write(ev);
        });
    }
    class track& operator[](unsigned int index) {
        for (auto& track : m_tracks)
            if (track.index() == index)
                return track;
        throw std::exception();
    }
    class track& track(unsigned int index) {
        return operator[](index);
    }

    void write_sync_aux(const mdev& event);
    void write_sync_dev(const mdev& event);
    mdbuf& async_buffer() { return m_async_buffer; }
    backend& set_backend(int type);
    void connect(std::string aux = {});

    class track& create_track(mdev ev = {});
    class track& create_track(layout l, mdev ev = {});
    void remove_track(mdev ev = {});

    void set_strip(midi_t mode, mdev ev = {});
    void screen_clearline(midi_t lineno, mdev ev = {});
    void screen_clear(mdev ev = {});
    void screen_display(midi_t row, midi_t col, std::string str, mdev ev = {});

    void set_button(midi_t index, midi_t mode, mdev ev = {});
    void set_toggle(midi_t row, midi_t index, midi_t mode, mdev ev = {});
    void set_pad(midi_t index, midi_t color, midi_t mode, mdev ev = {});

    void process_mdev(mdev& ev);
    void process_tick(ushort nframes);

private:
    void process_toggle(mdev& ev);
    void process_knob(mdev& ev);
    void process_button(mdev& ev);

    void mdwrite(midi_t status, midi_t b1, midi_t b2, mdev ev = {});
    void mdwrite(midi_t arr[], midi_t size, mdev ev, void *output);

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
