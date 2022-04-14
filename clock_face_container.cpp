// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// clock_face_container.cpp
// Implementation of the clock_face_container class
//

#include <jsoncpp/json/json.h>
#include <fstream>
#include "matrix_clock.h"

namespace matrix_clock {
    clock_face_container::clock_face_container(std::string config_file) {
        empty = new clock_face("~empty~");  // create an empty clock face in the background
        override_interface = false;
        force_update = false;
        clock_on = true;
        this->config_file = config_file;
    }

    void clock_face_container::update_clock_face(std::string name) {
        for (size_t i = 0; i < clock_faces.size(); i++) {      // loop through all clock faces
            if (!strcasecmp(clock_faces.data()[i]->get_name().c_str(), name.c_str())) {  // if we find one with a matching name, return it (case insensitive)
                current = clock_faces.data()[i];                                                // use !strcasecmp because it returns 0 if they are equal
                return;
            }
        }

        current = empty;       // set it to empty if not found
    }

    void clock_face_container::update_clock_face(int hour, int minute) {
        for (size_t index = 0; index < get_clock_face_count(); index++) {   // loop through all clock faces
            // grab the time periods for the current time period
            std::vector<matrix_clock::time_period> time_periods = clock_faces.data()[index]->get_time_periods();

            // loop through all time periods (THERE CAN BE MORE THAN ONE)
            for (size_t j = 0; j < time_periods.size(); j++) {
                matrix_clock::time_period current_period = time_periods.data()[j];

                if (current_period.in_time_period(hour, minute)) {  // check if the current time is within this time period
                    current = clock_faces.data()[index];
                    return; // return if found
                }   // otherwise, loop again until we find it
            }
        }

        current = empty;
    }

    std::string* clock_face_container::get_names(void) const {
        std::string* names_array = new std::string[get_clock_face_count()];  // create a new array with a length of the clock face count

        for (size_t i = 0; i < get_clock_face_count(); i++) {    // iterate through each clock face
            std::string name = clock_faces.data()[i]->get_name();    // grab the name of the clock face

            for (size_t j = 0; j < name.size(); j++) {  // iterate through each character
                if (j == 0)
                    name[j] = toupper(name[j]); // make the first one upper case
                else
                    name[j] = tolower(name[j]); // make every other one lower case
            }

            names_array[i] = name;
        }

        return names_array;
    }

    void clock_face_container::load_clock_faces() {
        // clear the current clock faces if they exist

        for (matrix_clock::clock_face* current_face : clock_faces) {    // delete all old clock faces
            delete current_face;
        }

        clock_faces.clear();    // ensure it is empty

        current = empty;        // set the current clock face to the empty one temporarily so it is never null

        // LOADING NEW DATA BELOW

        Json::Value jsonData;   // full json from the config file
        JSONCPP_STRING error;
        std::ifstream file_stream(config_file); // grab the matrix_config file

        Json::CharReaderBuilder builder;    // json value reader

        if (!parseFromStream(builder, file_stream, &jsonData, &error)) {
            file_stream.close();    // close file
            return;     // if the config file could not be parsed, return and the main program will kill the application
        }

        file_stream.close();    // file is valid and data has been loaded, close the file and begin parsing

        for (Json::Value::ArrayIndex face_index = 0; face_index != jsonData["clock_faces"].size(); face_index++) {  // loop through ALL clock face declared in the file
            std::string clock_face_name = jsonData["clock_faces"][face_index]["name"].asString();
            matrix_clock::clock_face *config_clock_face = new matrix_clock::clock_face(clock_face_name); // create a new clock face at the current index

            // loop through ALL time periods within the current interface
            for (Json::Value::ArrayIndex times_index = 0; times_index != jsonData["clock_faces"][face_index]["time_periods"].size(); times_index++) {
                Json::Value time_data = jsonData["clock_faces"][face_index]["time_periods"][times_index];

                int start_hour = time_data["start_hour"].asInt();       // grab fields from the time periods section of the clock face
                int start_minute = time_data["start_minute"].asInt();
                int end_hour = time_data["end_hour"].asInt();
                int end_minute = time_data["end_minute"].asInt();

                // instantiate a new time period and add it to the clock face
                matrix_clock::time_period clock_face_time_period(start_hour, start_minute, end_hour, end_minute);

                config_clock_face->add_time_period(clock_face_time_period); // add the time frame to the clock face object
            }

            // now we are going to loop through all text lines
            for (Json::Value::ArrayIndex text_index = 0; text_index != jsonData["clock_faces"][face_index]["text_lines"].size(); text_index++) {
                Json::Value text_data = jsonData["clock_faces"][face_index]["text_lines"][text_index];

                matrix_clock::matrix_color color;       // variables that will need to be filled
                matrix_clock::matrix_font font_size;
                int x_pos, y_pos;
                std::string text;

                if (text_data["color"]["built_in_color"].asString() == "none") {    // if a prebuilt color is NOT USED (denoted "none" in config), load in RGB values
                    int red = text_data["color"]["r"].asInt();
                    int green = text_data["color"]["g"].asInt();
                    int blue = text_data["color"]["b"].asInt();

                    color = matrix_clock::matrix_color(red, green, blue);
                } else {                                                            // a prebuilt color is used, read it in from string
                    std::string prebuilt_color_name = text_data["color"]["built_in_color"].asString();
                    color = matrix_clock::matrix_color(prebuilt_color_name);
                }

                font_size = matrix_clock::font::font_from_string(text_data["font_size"].asString());    // grab font size, positioning, and text
                x_pos = text_data["x_position"].asInt();
                y_pos = text_data["y_position"].asInt();
                text = text_data["text"].asString();

                if (text.find("{second}") != std::string::npos)         // if there is a second in the variables, let the clock face know
                    config_clock_face->set_contains_second_variable(true);      // in this scenario we need to update the screen secondly instead of minutely

                matrix_clock::text_line clock_face_text_line(color, font_size, x_pos, y_pos, text); // instantiate the text line object

                config_clock_face->add_text(clock_face_text_line);      // add the text line to the current clock face
            }

            add_clock_face(config_clock_face);        // add the clock face to the container
        }
    }
}
