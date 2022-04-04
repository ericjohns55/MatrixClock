// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// matrix_color.cpp
// Implementation of the matrix_color class
//

#include "matrix_clock.h"

namespace matrix_clock {
    matrix_color::matrix_color(int red, int green, int blue) {
        this->r = red;      // basic constructor, save values to field variables
        this->g = green;
        this->b = blue;
    }

    void matrix_color::parse_color(matrix_prebuilt_colors hardcoded_color) {
        switch (hardcoded_color) {      // i decided on these rgb values out of personal preference
            case red:                   // convert our enums to rgb values
                r = 128;
                break;
            case orange:
                r = 253;
                g = 88;
                break;
            case yellow:
                r = 255;
                g = 228;
                break;
            case green:
                g = 160;
                break;
            case blue:
                g = 64;
                b = 255;
                break;
            case purple:
                r = b = 128;
                break;
            case pink:
                r = b = 228;
                break;
            case white:
                r = g = b = 255;
                break;
            case gray:
                r = g = b = 128;
                break;
            case black:
                r = g = b = 0;
                break;
            case brown:
                r = 101;
                g = 67;
                b = 33;
                break;
            case night_time:
                r = 128;
                break;      // no default needed because it cannot be anything else
        }
    }
}