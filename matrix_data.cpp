// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// matrix_data.cpp
// Implementation of the matrix_data class
//

#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>
#include "matrix_clock.h"

namespace matrix_clock {
    matrix_data::matrix_data(std::string config_file) {
        empty = new clock_face("~empty~", matrix_color(matrix_prebuilt_colors::black));  // create an empty clock face in the background
        override_interface = false;
        force_update = false;
        clock_on = true;
        this->config_file = config_file;
    }

    void matrix_data::update_clock_face(std::string name) {
        for (size_t i = 0; i < clock_faces.size(); i++) {      // loop through all clock faces
            if (!strcasecmp(clock_faces.data()[i]->get_name().c_str(), name.c_str())) {  // if we find one with a matching name, return it (case insensitive)
                current = clock_faces.data()[i];                                                // use !strcasecmp because it returns 0 if they are equal
                return;
            }
        }

        current = empty;       // set it to empty if not found
    }

    void matrix_data::update_clock_face(int hour, int minute) {
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

    std::string* matrix_data::get_names(void) const {
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

    bool matrix_data::load_clock_data() {
        // clear the current clock faces if they exist

        for (matrix_clock::clock_face* current_face : clock_faces) {    // delete all old clock faces
            delete current_face;
        }

        clock_faces.clear();    // ensure it is empty

        for (telegram_push* current_push : push_notifications) {        // do the same thing with push notifications
            delete current_push;
        }

        push_notifications.clear();

        current = empty;        // set the current clock face to the empty one temporarily so it is never null

        // LOADING NEW DATA BELOW

        try {   // attempt to load data
            Json::Value jsonData;   // full json from the config file
            JSONCPP_STRING error;
            std::ifstream file_stream(config_file); // grab the matrix_config file

            Json::CharReaderBuilder builder;    // json value reader

            if (!parseFromStream(builder, file_stream, &jsonData, &error)) {
                file_stream.close();    // close file
                std::cerr << "Invalid JSON file provided." << std::endl;
                return false;           // if the config file could not be parsed, return and the main program will kill the application
            }

            file_stream.close();    // file is valid and data has been loaded, close the file and begin parsing

            // clock data value, this is where the weather URL and the bot tokens are stored
            Json::Value clock_data = jsonData["clock_data"];

            // grab weather URL from the config file
            weather_url = clock_data["weather_url"].asString();

            // grab bot token from config file
            bot_token = clock_data["bot_token"].asString();

            // grab telegram chat ID for the bot from config file
            // the user can find this by clicking on "Chat ID" in the inline keyboard menu within the bot
            bot_chat_id = clock_data["chat_id"].asInt();

            for (Json::Value::ArrayIndex face_index = 0; face_index != jsonData["clock_faces"].size(); face_index++) {  // loop through ALL clock face declared in the file
                Json::Value clock_face_data = jsonData["clock_faces"][face_index];
                std::string name = clock_face_data["name"].asString();

                matrix_clock::matrix_color bg_color;

                if (clock_face_data["bg_color"]["built_in_color"].asString() == "none") {    // if a prebuilt color is NOT USED (denoted "none" in config), load in RGB values
                    int red = clock_face_data["bg_color"]["r"].asInt();
                    int green = clock_face_data["bg_color"]["g"].asInt();
                    int blue = clock_face_data["bg_color"]["b"].asInt();

                    bg_color = matrix_clock::matrix_color(red, green, blue);
                } else {                                                            // a prebuilt color is used, read it in from string
                    std::string prebuilt_color_name = clock_face_data["bg_color"]["built_in_color"].asString();
                    bg_color = matrix_clock::matrix_color(prebuilt_color_name);
                }

                // create a new clock face at the current index with the given name and background color
                matrix_clock::clock_face* config_clock_face = new matrix_clock::clock_face(name, bg_color);

                // loop through ALL time periods within the current interface
                for (Json::Value::ArrayIndex times_index = 0; times_index != clock_face_data["time_periods"].size(); times_index++) {
                    Json::Value time_data = clock_face_data["time_periods"][times_index];

                    int start_hour = time_data["start_hour"].asInt();       // grab fields from the time periods section of the clock face
                    int start_minute = time_data["start_minute"].asInt();
                    int end_hour = time_data["end_hour"].asInt();
                    int end_minute = time_data["end_minute"].asInt();

                    // instantiate a new time period and add it to the clock face
                    matrix_clock::time_period clock_face_time_period(start_hour, start_minute, end_hour, end_minute);

                    config_clock_face->add_time_period(clock_face_time_period); // add the time frame to the clock face object
                }

                // now we are going to loop through all text lines
                for (Json::Value::ArrayIndex text_index = 0; text_index != clock_face_data["text_lines"].size(); text_index++) {
                    Json::Value text_data = clock_face_data["text_lines"][text_index];

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

            // now we are going to read the telegram notifications box from the config file
            Json::Value notifications = jsonData["telegram_notifications"];

            for (Json::Value::ArrayIndex noti_index = 0; noti_index != notifications.size(); noti_index++) {
                std::string message = notifications[noti_index]["message"].asString();  // read the message, hour, and minute
                int hour = notifications[noti_index]["hour"].asInt();
                int minute = notifications[noti_index]["minute"].asInt();

                std::vector<int> days;

                // loop through the days of the week array and add it to the vector
                for (Json::Value::ArrayIndex days_index = 0; days_index != notifications[noti_index]["days_of_week"].size(); days_index++) {
                    days.push_back(notifications[noti_index]["days_of_week"][days_index].asInt());
                }

                telegram_push* push_notification = new telegram_push(message, hour, minute, days);
                add_notification(push_notification);        // create the new object and push back
            }

            return true;        // Return true because we successfully parsed the file
        } catch (const Json::Exception& exception) {    // if data could not be loaded, return false so main kills the program - we need valid data to be able to load the clock faces
            std::cerr << "Could not parse JSON values: " << exception.what() << std::endl;  // print out the error to help the user find their error
            return false;
        }
    }
}

