package push

ColorScheme :: struct {
       dark: midi_t,
     medium: midi_t,
     bright: midi_t,
    pressed: midi_t
}

Layout :: struct {
    pressed: midi_t,
     colors: [12]midi_t
}

SCHEME_DEFAULT :: ColorScheme {
       dark = 0,
     medium = 101,
     bright = 86,
    pressed = 16
};

LAYOUT_DEFAULT :: Layout {
    pressed = SCHEME_DEFAULT.pressed,
    colors = {
        SCHEME_DEFAULT.bright,
        SCHEME_DEFAULT.dark,
        SCHEME_DEFAULT.medium,
        SCHEME_DEFAULT.dark,
        SCHEME_DEFAULT.medium,
        SCHEME_DEFAULT.medium,
        SCHEME_DEFAULT.dark,
        SCHEME_DEFAULT.medium,
        SCHEME_DEFAULT.dark,
        SCHEME_DEFAULT.medium,
        SCHEME_DEFAULT.dark,
        SCHEME_DEFAULT.medium
    }
};
