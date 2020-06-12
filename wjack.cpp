#include "wjack.hpp"

using namespace wpn214::push;

jack_backend::jack_backend(device& dev) : backend(dev)
{
    m_client = jack_client_open(WPUSH_JACK_NAME, JackNullOption, nullptr);
    m_dev_in = jack_port_register(m_client, "dev_in", WJACK_MIDI, WJACK_INPUT, 0);
    m_aux_in = jack_port_register(m_client, "aux_in", WJACK_MIDI, WJACK_INPUT, 0);
    m_dev_out = jack_port_register(m_client, "dev_out", WJACK_MIDI, WJACK_OUTPUT, 0);
    m_aux_out = jack_port_register(m_client, "aux_out", WJACK_MIDI, WJACK_OUTPUT, 0);

    assert(m_dev_in && m_aux_in && m_dev_out && m_aux_out);
    jack_set_process_callback(m_client, &jack_backend::process, this);
}

int jack_backend::process(jack_nframes_t nframes, void* udata)
{
    auto j = static_cast<jack_backend*>(udata);
    jack_midi_event_t jmev;
    void* dev_in  = jack_port_get_buffer(j->m_dev_in, nframes);
    void* aux_in  = jack_port_get_buffer(j->m_aux_in, nframes);
    void* dev_out = jack_port_get_buffer(j->m_dev_out, nframes);
    void* aux_out = jack_port_get_buffer(j->m_aux_out, nframes);
    uint nev = jack_midi_get_event_count(dev_in);
    jack_midi_clear_buffer(dev_out);
    jack_midi_clear_buffer(aux_out);

    mdev* ev = nullptr;
    auto& mdbuf = j->m_dev.async_buffer();
    while ((ev = mdbuf.next())) {
        jack_midi_data_t* mdt;
        mdt = jack_midi_event_reserve(dev_out, 0, ev->nbytes);
        memcpy(mdt, ev->data, ev->nbytes);
    }
    mdbuf.clear();
    j->m_dev.process_tick(nframes);

    // process incoming midi events (from device)
    for (uint n = 0; n < nev; ++n) {
        struct mdev mdev;
        jack_midi_event_get(&jmev, dev_in, n);
        mdev.nbytes = jmev.size;
        mdev.frameno = jmev.time;
        mdev.data = jmev.buffer;
        j->m_dev.process_mdev(mdev);
    }
    return 0;
}

void jack_backend::start()
{
    const char **dev_i, **dev_o;
    dev_i = jack_get_ports(m_client, WPUSH_DEV_NAME, WJACK_MIDI, WJACK_INPUT);
    dev_o = jack_get_ports(m_client, WPUSH_DEV_NAME, WJACK_MIDI, WJACK_OUTPUT);
    assert(dev_i && dev_o);
    jack_activate(m_client);
    jack_connect(m_client, jack_port_name(m_dev_out), dev_i[1]);
    jack_connect(m_client, dev_o[1], jack_port_name(m_dev_in));

    // aux connect
    if (!m_aux.empty()) {
        dev_i = jack_get_ports(m_client, m_aux.c_str(), WJACK_MIDI, WJACK_INPUT);
        dev_o = jack_get_ports(m_client, m_aux.c_str(), WJACK_MIDI, WJACK_OUTPUT);
        if (dev_i && dev_o) {
            jack_connect(m_client, jack_port_name(m_aux_out), dev_i[0]);
        }
    }
}
