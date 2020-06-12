#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string.h>
#include <cassert>

using ushort = unsigned short;
#define rcast reinterpret_cast
#define scast static_cast

namespace wpn214 {

using byte_t = uint8_t;
using midi_t = uint8_t;

/** simple struct representing a single midi event with variable data size. */
struct mdev {
    midi_t frameno;
    midi_t nbytes;
    midi_t* data;
};

class mdbuf {
public:
    mdbuf() = delete;
    mdbuf(ushort nbytes) :
        m_capacity(nbytes),
        m_data(new byte_t[nbytes]()) {
        clear();
    }
    ~mdbuf() {
        delete[] m_data;
    }
    void clear() noexcept {
        memset(m_data, 0, m_capacity);
    }
    mdev& pull(ushort nbytes) {
        ushort w = m_w.load();
        ushort nwi = w+nbytes+sizeof(mdev);
        if (nwi > m_capacity)
            assert(false); // throw
        else {
            mdev& event = *(rcast<mdev*>(&m_data[w]));
            event.frameno = 0;
            event.nbytes = nbytes;
            event.data = &m_data[w+sizeof(mdev)];
            m_w.store(nwi);
            return event;
        }
    }
    void write(mdev ev) {
        auto& target = pull(ev.nbytes);
        memcpy(target.data, ev.data, ev.nbytes);
    }

    mdev* next() noexcept {
        ushort r = m_r.load();
        if (r == m_capacity || r == m_w.load())
            return nullptr;
        else {
            auto ev = rcast<mdev*>(&m_data[r]);
            r += sizeof(mdev)+ev->nbytes;
            m_r.store(r);
            return ev;
        }
    }
private:
    std::atomic_ushort m_r {0};
    std::atomic_ushort m_w {0};
    ushort m_capacity;
    byte_t* m_data;
};
}
