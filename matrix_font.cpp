// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// matrix_font.cpp
// Helper methods for the matrix_font enum
//

#include <iostream>
#include <sstream>
#include <fstream>
#include "matrix_clock.h"

namespace matrix_clock {
    matrix_font::matrix_font(matrix_built_in_font built_font) {
        switch (built_font) {           // convert our built in fonts
            case small:
                font_size = "5x8";      break;
            case medium:
                font_size = "6x9";      break;
            case large:
                font_size = "8x13";     break;
            case large_bold:
                font_size = "8x13B";    break;
            default:
                font_size = "6x9";      break;
        }
    }

    int matrix_font::get_x() const {
        return std::stoi(font_size.substr(0, font_size.find("x"))); // find the x value in the string by taking the substring of the beginning to anything before the 'x'
    }

    void matrix_font::parse_font(std::string font_folder, std::string font) {
        if (font == "small") {      // check out prebuilt fonts first, these we do not need to parse because we know them to be correct
            font_size = "5x8";
        } else if (font == "medium") {
            font_size = "6x9";
        } else if (font == "large") {
            font_size = "8x13";
        } else if (font == "large_bold") {
            font_size = "8x13B";
        } else {
            std::ifstream stream(get_font_file(font_folder, font));      // otherwise it is a custom one, check if the file is valid and set to a default value if not
            if (!stream.good()) {
                std::cout << "Could not find font " << font << ", loading medium font (6x9.bdf) instead." << std::endl;
                this->font_size = "6x9";
            } else {
                this->font_size = font;     // valid font, we can use it with no change necessary
            }
        }
    }

    std::string matrix_font::get_font_file(std::string font_folder, std::string font_size) {
        std::stringstream file_builder;
        file_builder << font_folder << "/" << font_size << ".bdf"; // build the font path using a stringstream and return it
        return file_builder.str();
    }
}