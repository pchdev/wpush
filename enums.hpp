#pragma once

namespace wpn214 {
namespace push {

enum mode {
    Off               = 0,
    RedDim            = 1,
    RedDimSlow        = 2,
    RedDimFast        = 3,
    RedFull           = 4,
    RedFullSlow       = 5,
    RedFullFast       = 6,
    OrangeDim         = 7,
    OrangeDimSlow     = 8,
    OrangeDimFast     = 9,
    OrangeFull        = 10,
    OrangeFullSlow    = 11,
    OrangeFullFast    = 12,
    YellowDim         = 13,
    YellowDimSlow     = 14,
    YellowDimFast     = 15,
    YellowFull        = 16,
    YellowFullSlow    = 17,
    YellowFullFast    = 18,
    GreenDim          = 19,
    GreenDimSlow      = 20,
    GreenDimFast      = 21,
    GreenFull         = 22,
    GreenFullSlow     = 23,
    GreenFullFast     = 24
};

enum ribbon {
    Bend        = 0,
    Modwheel    = 1,
    Pan         = 2,
    Volume      = 3
};

enum sensors {
    Knob_0,
    Knob_1,
    Knob_2,
    Knob_3,
    Knob_4,
    Knob_5,
    Knob_6,
    Knob_7,
    Knob_8,
    Swing,
    Metronome,
    Ribbon = 12,
};

namespace button {
enum {
    Play           = 85,
    Record         = 86,
    New            = 87,
    Duplicate      = 88,
    Automation     = 89,
    FixedLength    = 90,
    Quantize       = 116,
    Double         = 117,
    Delete         = 118,
    Undo           = 119,
    Metronome      = 9,
    TapTempo       = 3,
    LeftArrow      = 44,
    RightArrow     = 45,
    UpArrow        = 46,
    DownArrow      = 47,
    Select         = 48,
    Shift          = 49,
    Note           = 50,
    Session        = 51,
    AddEffect      = 52,
    AddTrack       = 53,
    OctaveDown     = 54,
    OctaveUp       = 55,
    Repeat         = 56,
    Accent         = 57,
    Scales         = 58,
    User           = 59,
    Mute           = 60,
    Solo           = 61,
    Next           = 62,
    Previous       = 63,
    Device         = 110,
    Browse         = 111,
    Track          = 112,
    Clip           = 113,
    Volume         = 114,
    PanSend        = 115,
    Div4th         = 36,
    Div4th_t       = 37,
    Div8th         = 38,
    Div8th_t       = 39,
    Div16th        = 40,
    Div16th_t      = 41,
    Div32nd        = 42,
    Div32nd_t      = 43
};

enum mode {
    Off        = 0,
    Dim        = 1,
    DimSlow    = 2,
    DimFast    = 3,
    Full       = 4,
    FullSlow   = 5,
    FullFast   = 6
};
}

namespace pad {
enum mode {
    Normal     = 0,
    Fade_24    = 1,
    Fade_16    = 2,
    Fade_8     = 3,
    Fade_4     = 4,
    Fade_2     = 5,
    Pulse_24   = 6,
    Pulse_16   = 7,
    Pulse_8    = 8,
    Pulse_4    = 9,
    Pulse_2    = 10,
    Blink_24   = 11,
    Blink_16   = 12,
    Blink_8    = 13,
    Blink_4    = 14,
    Blink_2    = 15
};

enum color {
    Black         = 0,
    White_1       = 1,
    White_2       = 2,
    White_3       = 3,
    Salmon        = 4,
    Red_1         = 5,
    Red_2         = 6,
    Red_3         = 7,
    Beige         = 8,
    Orange_1      = 9,
    Orange_2      = 10,
    Orange_3      = 11,
    Gold          = 12,
    Yellow_1      = 13,
    Yellow_2      = 14,
    Yellow_3      = 15,
    Green_1       = 16,
    Green_2       = 17,
    Green_3       = 18,
    Green_4       = 19,
    Green_5       = 20,
    Green_6       = 21,
    Green_7       = 22,
    Green_8       = 23,
    Green_9       = 24,
    Green_10      = 25,
    Green_11      = 26,
    Green_12      = 27,
    Blue_1        = 28,
    Blue_2        = 29,
    // ... more TODO
};
}

enum toggle {
    // upper row will go from 20 to 27 (included)
    // lower row will go from 102 to 109 (included)
    UpperFirst    = 20,
    UpperLast     = 27,
    LowerFirst    = 102,
    LowerLast     = 109
};
}
}
