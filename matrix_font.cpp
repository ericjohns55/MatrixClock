// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// matrix_font.cpp
// Helper methods for the matrix_font enum
//

#include "matrix_clock.h"

namespace matrix_clock {
    int font::get_x(matrix_font font_size) {
        switch (font_size) {        // i hardcoded 4 fonts, these are the widths of each
            case small:
                return 5;
            case medium:
                return 6;
            case large:
                return 8;
            case large_bold:
                return 8;
            default:
                return 0;
        }
    }

    std::string font::parse_font(matrix_font font_size) {   // grab the fonts from the rpi-rgb-led-matrix's font library
        switch (font_size) {                                // it is assumed that this program will be cloned into the matrix library's repo folder
            case small:
                return "../fonts/5x8.bdf";
            case medium:
                return "../fonts/6x9.bdf";
            case large:
                return "../fonts/8x13.bdf";
            case large_bold:
                return "../fonts/8x13B.bdf";
            default:
                return "../fonts/6x9.bdf";  // default to medium if something goes wrong (should not be possible)
        }
    }

    matrix_font font::font_from_string(std::string font_size) {
        if (font_size == "small") {     // i considered mapping these conversions from string to enum, however with 4 cases i thought this was more efficient
            return matrix_font::small;
        } else if (font_size == "medium") {
            return matrix_font::medium;
        } else if (font_size == "large") {
            return matrix_font::large;
        } else {
            return matrix_font::large_bold;
        }
    }
}