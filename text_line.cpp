// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// text_line.cpp
// Implementation of the text_line class
//

#define MATRIX_WIDTH 64

#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>

#include "matrix_clock.h"

// replace_string(std::string& source, std::string search, std::string replace)
//      replace all instances of "search" in the source string with "replace"
//
//      source = the string to execute a replace on
//      search = the string we want to have replaced
//      replace = what we want to the search term with
void replace_string(std::string& source, std::string search, std::string replace);

// pad_numbers(int source)
//      pad the source number to be two digits with a leading 0
//      this exists mostly for standard readability of the minutes for the clock
//
//      source = the integer to add a leading 0 in front of (if necessary, if the source is greater than 10 than it does nothing but convert to string)
std::string pad_numbers(int source);

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
        parsed_text = text; // load current text with variables into the parsed text

        int times[4];       // load times from data object
        util->get_time(times);

        // perform text replacements using the parsed text field and defined variables
        // fix formatting where necessary (padding 0s and converting time to am or pm)
        replace_string(parsed_text, "{hour}", std::to_string(times[0]));
        replace_string(parsed_text, "{minute}", pad_numbers(times[1]));
        replace_string(parsed_text, "{second}", pad_numbers(times[2]));
        replace_string(parsed_text, "{hour24}", std::to_string(times[3]));
        replace_string(parsed_text, "{ampm}", times[3] < 12 ? "am" : "pm");
        replace_string(parsed_text, "{temp}", std::to_string(util->get_temp()));
        replace_string(parsed_text, "{temp_feel}", std::to_string(util->get_real_feel()));
        replace_string(parsed_text, "{humidity}", std::to_string(util->get_humidity()));
        replace_string(parsed_text, "{forecast}", util->get_forecast());
        replace_string(parsed_text, "{forecast_short}", util->get_forecast_short());
        replace_string(parsed_text, "{date_format}", util->get_formatted_date());
        replace_string(parsed_text, "{month_name}", util->get_month_name());
        replace_string(parsed_text, "{day_name}", util->get_day_name());
        replace_string(parsed_text, "{month_num}", std::to_string(util->get_month_num()));
        replace_string(parsed_text, "{month_day}", std::to_string(util->get_day_of_month()));
        replace_string(parsed_text, "{week_day_num}", std::to_string(util->get_day_of_week()));
        replace_string(parsed_text, "{year}", std::to_string(util->get_year()));

        // for wind speed, we are truncating to 1 decimal place for easier readability (nobody cares how exact it is)
        std::string wind_speed = std::to_string(util->get_wind_speed());
        replace_string(parsed_text, "{wind_speed}",
                       wind_speed.substr(0, wind_speed.find(".") + 2));

        font font_parser;
        if (parsed_text.size() * font_parser.get_x(font_size) > MATRIX_WIDTH) { // do same thing as we did in parse_x(), make sure all text can fit on the screen and truncate what does not
            parsed_text = parsed_text.substr(0, (int) MATRIX_WIDTH / font_parser.get_x(font_size));
        }
    }
}

std::string pad_numbers(int source) {
    std::stringstream stream;

    if (source < 10) {
        stream << "0";  // if the integer is less than 10, add in a zero infront for readability in time
    }

    stream << std::to_string(source);

    return stream.str();
}

void replace_string(std::string& source, std::string search, std::string replace) {
    size_t position = source.find(search);  // find the current index of the search string

    while (position != std::string::npos) { // loop until it cannot be found anymore (replaced out), will not run with 0 instances
        source.replace(position, search.size(), replace);   // replace at the position
        position = source.find(search, position + replace.size());  // find next instance (will not loop again with none found)
    }
}