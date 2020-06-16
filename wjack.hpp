#pragma once
#include <jack/jack.h>
#include <jack/midiport.h>
#include "wpush.hpp"

#define WPUSH_DEV_NAME      "Ableton Push"
#define WPUSH_DEV_PORT      "(playback): Ableton Push MIDI 2"
#define WPUSH_JACK_NAME     "wpush214"
#define WJACK_MIDI          JACK_DEFAULT_MIDI_TYPE
#define WJACK_INPUT         JackPortIsInput
#define WJACK_OUTPUT        JackPortIsOutput

namespace wpn214 {
namespace push {

class jack_backend : public backend {
public:
    jack_backend(device& dev);
    ~jack_backend() {
        jack_deactivate(m_client);
        jack_client_close(m_client);
    }
    void set_aux(std::string aux) override { m_aux = aux; }
    void connect() override {}
    void start() override;
    void stop() override { jack_deactivate(m_client); }

    mdwriter::mdw_fn wsync_fn() const override {
        return [this](mdev const& ev, void* output) {            
            void* buf = jack_port_get_buffer(
                       (jack_port_t*)output, m_nframes);
            jack_midi_data_t* mdt;
            mdt = jack_midi_event_reserve(buf, ev.frameno, ev.nbytes);
            memcpy(mdt, ev.data, ev.nbytes);
            if (m_log && ev.nbytes == 3) {
                printf("[midi-sync] status: %d, index: %d, value: %d\n",
                       ev.data[0], ev.data[1], ev.data[2]);
            }
        };
    }

    void* dev_out_data() override { return m_dev_out; }
    void* aux_out_data() override { return m_aux_out; }
    static int process(jack_nframes_t nframes, void* udata);

private:
    bool m_log = true;
    jack_client_t* m_client = nullptr;
    jack_port_t* m_dev_in = nullptr;
    jack_port_t* m_aux_in = nullptr;
    jack_port_t* m_dev_out = nullptr;
    jack_port_t* m_aux_out = nullptr;
    std::string m_aux;
    jack_nframes_t m_nframes = 0;
};
}
}
