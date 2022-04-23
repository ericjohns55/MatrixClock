// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// text_line.cpp
// Implementation of the text_line class
//

#define MATRIX_WIDTH 64

#include <string>

#include "matrix_clock.h"

namespace matrix_clock {
    const rgb_matrix::Color text_line::get_color(void) const {  // convert matrix_color to the rgb library's matrix color
        return rgb_matrix::Color(color.get_red(), color.get_green(), color.get_blue());
    }

    // basic constructor to instantiate all fields, used when generating the objects from the json file
    text_line::text_line(matrix_color color, matrix_font font_size, int x_pos, int y_pos, std::string text) {
        this->color = color;
        this->font_size = font_size;
        this->x_pos = x_pos;
        this->y_pos = y_pos;
        this->text = text;
    }

    int text_line::parse_x() {
        font font_parser;

        // if the position is -1, then we want to center
        if (x_pos == -1) {  // calculate using the width minus the (size * font width) all over 2 for the starting x
            return (MATRIX_WIDTH - (parsed_text.size() * font_parser.get_x(font_size))) / 2;
        } else {    // they chose their x, make sure everything fits on the page, (check if parsed text size * font_width is greater than the width - start x)
            if (((int) parsed_text.size()) * font_parser.get_x(font_size) > (MATRIX_WIDTH - x_pos)) {
                parsed_text = parsed_text.substr(0, (int) ((MATRIX_WIDTH - x_pos) / font_parser.get_x(font_size)));
            }   // create substring of only the characters that will fit on the screen

            return x_pos;
        }
    }

    void text_line::parse_variables(matrix_clock::variable_utility* util) {
        parsed_text = util->parse_variables(text); // parse the variables into the new text

        // cut the string down if we know it will not fit on the screen
        font font_parser;
        if (parsed_text.size() * font_parser.get_x(font_size) > MATRIX_WIDTH) { // do same thing as we did in parse_x(), make sure all text can fit on the screen and truncate what does not
            parsed_text = parsed_text.substr(0, (int) MATRIX_WIDTH / font_parser.get_x(font_size));
        }
    }
}