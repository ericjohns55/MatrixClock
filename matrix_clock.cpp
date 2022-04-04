// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// matrix_clock.cpp
// The main program that consists of instantiating the clock, reading data from file, writing it to the screen, and updating
// the data when needed
//

#include <chrono>
#include <thread>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <jsoncpp/json/json.h>
#include "matrix_clock.h"

#include "led-matrix.h"
#include "graphics.h"

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

using namespace std;

// boolean that checks if there is an interrupt pushed to the system
volatile bool interrupt_received = false;

// load_clock_faces()
//      this loads all the clock faces from the matrix_config.json in the repository
//      you can edit matrix_config.json all you want, as long as valid json and data is submitted than it will load the
//      program using that data and update everything when needed
//      this MUST BE called before you attempt to write any data to the screen
//      otherwise nothing will be loaded and there will be no information to grab for writing
matrix_clock::clock_face_container load_clock_faces(string config_file);

// get_clock_face(matrix_clock::clock_face_container clock_faces, int hour, int minute)
//      this takes in a clock_face_container, the current hour, and current minute and grabs the clock face embedded within the current time
//      if two time frames overlap, it will return whatever one is returned first in matrix_config.json
//      the clock_face returned will have the current hour and minute within a time_period in the object
matrix_clock::clock_face* get_clock_face(matrix_clock::clock_face_container clock_faces, int hour, int minute);

// Control-C interrupt to kill the program
static void InterruptHandler(int signal) {
    interrupt_received = true;
}

// update clock method
// take in the offscreen canvas to draw to
// the clock_face is the current clock face shown on the screen
// variable utility is passed in to parse variables against
void update_clock(rgb_matrix::FrameCanvas* offscreen, matrix_clock::clock_face* clock_face, matrix_clock::variable_utility* util) {
    offscreen->Clear(); // clear offscreen because it was previously swapped

    for (int i = 0; i < clock_face->get_line_count(); i++) {    // loop through all lines to render
        matrix_clock::text_line current_line = clock_face->get_line(i);  // grab current line from the clock face
        current_line.parse_variables(util);     // parse the variables into actual data

        rgb_matrix::Font font; // create a font parser object
        font.LoadFont(matrix_clock::font::parse_font(current_line.get_font()).c_str());

        // draw the text using the color, positionings, and font size declared on the off screen campus
        rgb_matrix::DrawText(offscreen, font, current_line.parse_x(), current_line.get_y(),
                             current_line.get_color(), current_line.get_parsed_text().c_str());
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) { // make sure the minimum amount of arguments were provided for the program to run
        cerr << "Only " << argc << " arguments provided:" << endl;
        cerr << "Usage: " << argv[0] << " --WEATHER_URL <weather url> --CONFIG_FILE <config file location>" << endl;
        return EXIT_FAILURE;
    }

    string config_file;     // we are going to load both the config file path and the weather url the command arguments
    string weather_url;

    for (int i = 1; i < argc; i++) {    // loop through all the given arguments
        if (string(argv[i]) == "--WEATHER_URL") {   // check if we found a weather URL specifier
            if (i + 1 < argc) {                        // make sure there is at least one more argument past our current position
                weather_url = argv[++i];               // load weather url from arguments
            } else {                                   // otherwise we have not found an argument, tell the user
                cerr << "--WEATHER_URL requires an argument" << endl;
            }
        } else if (string(argv[i]) == "--CONFIG_FILE") {    // do the same thing as before but for the configuration file
            if (i + 1 < argc) {
                config_file = argv[++i];
            } else {
                cerr << "--CONFIG_FILE requires an argument" << endl;
            }
        }
    }

    if (config_file.empty() || config_file.empty()) // if either file is empty, count on the previous error messages saying what is wrong
        return EXIT_FAILURE;                        // kill the program

    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "adafruit-hat-pwm";     // these are the most optimized hardware options for my screen
    defaults.rows = 64;                                 // yours may be different
    defaults.cols = 64;
    defaults.pwm_lsb_nanoseconds = 130;
    defaults.brightness = 50;
    defaults.limit_refresh_rate_hz = 300;
//    defaults.show_refresh_rate = true;                // for debugging purposes

    // load more flags from command line that you cannot in the code
    // for example, i use --led-gpio-slowdown in the program arguments and it pulls in there
    RGBMatrix *matrix = RGBMatrix::CreateFromFlags(&argc, &argv, &defaults);

    if (matrix == NULL) {   // kill the program if we cannot find the matrix
        cerr << "Could not create matrix" << endl;
        return EXIT_FAILURE;
    }

    signal(SIGTERM, InterruptHandler); // declare interrupts for Control-C
    signal(SIGINT, InterruptHandler);

    rgb_matrix::FrameCanvas* offscreen = matrix->CreateFrameCanvas();   // create an offscreen canvas

    matrix_clock::clock_face_container clock_faces = load_clock_faces(config_file);    // load the clock faces from json

    if (clock_faces.get_clock_face_count() == 0) {  // check if none loaded
        cerr << "Invalid configuration file" << endl;   // kill the program because we cannot have 0 interfaces
        return EXIT_FAILURE;
    }

    matrix_clock::variable_utility time_util(weather_url);   // generate a time util

    time_util.poll_date();  // on first run, poll date and weather because they have not been loaded yet
    time_util.poll_weather();

    int times[4];           // declare a times array to frequently update
    time_util.get_time(times);

    // load initial clock face
    matrix_clock::clock_face* clock_face_pointer = get_clock_face(clock_faces, times[3], times[1]);

    if (clock_face_pointer == nullptr) {    // invalid configuration file, all times are not covered
        cerr << "Could not find a clock interface at the current time." << endl;
        cerr << "Please check your configuration file to make sure all times of day are covered." << endl;
        return EXIT_FAILURE;
    } else {    // valid clock face and other data, go ahead and update then swap out the on screen and off-screen canvases
        update_clock(offscreen, clock_face_pointer, &time_util);
        offscreen = matrix->SwapOnVSync(offscreen);
    }

    // declare the previous second
    int previous_second = times[2];

    while (!interrupt_received) { // loop until the program is killed
        time_util.get_time(times);  // update our times variable

        int new_second = times[2];  // check the new second

        if (previous_second != new_second) {    // only run the following code if the seconds have changed, otherwise sleep for 0.2 seconds
            bool new_minute = times[2] == 0;    // create boolean for if the minute changed
            previous_second = new_second;       // update previous second for next loop

            if (time_util.is_new_day())         // if the day has changed, poll the new date data
                time_util.poll_date();

            if (times[1] % 5 == 0 && new_minute)    // if the minute is a multiple of 5, update weather info (weather API has a free polling limit, so i only update once every 5 minutes)
                time_util.poll_weather();

            if (new_minute) // if there is a new minute, grab the interface again in case it changed (interfaces cannot change on a second)
                clock_face_pointer = get_clock_face(clock_faces, times[3], times[1]);

            if (clock_face_pointer == nullptr) {    // same problem as before, there is not an interface for the current time so kill the program
                cerr << "Could not find a clock interface at the current time.";
                return EXIT_FAILURE;
            }

            // otherwise, update every second if:
            //      1) we have a second count displayed on the screen that must update
            //      2) there is no second count, BUT it is a new minute so we have to update anyways
            // do not update under ANY OTHER CIRCUMSTANCES
            if (clock_face_pointer->contains_second_variable() || (!clock_face_pointer->contains_second_variable() && new_minute)) {
                update_clock(offscreen, clock_face_pointer, &time_util);
                offscreen = matrix->SwapOnVSync(offscreen);
            }
        }

        // sleep for 0.2 seconds and check again
        // we do this because C++ system time is not the most accurate
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // free up the matrix memory
    delete matrix;

    return EXIT_SUCCESS;
}

// this is where we get the current clock interface for the minute
matrix_clock::clock_face* get_clock_face(matrix_clock::clock_face_container clock_faces, int hour, int minute) {
    for (size_t index = 0; index < clock_faces.get_clock_face_count(); index++) {   // loop through all clock faces
        // grab the time periods for the current time period
        vector<matrix_clock::time_period> time_periods = clock_faces.get_clock_face(index)->get_time_periods();

        // loop through all time periods (THERE CAN BE MORE THAN ONE)
        for (size_t j = 0; j < time_periods.size(); j++) {
            matrix_clock::time_period current_period = time_periods.data()[j];

            if (current_period.in_time_period(hour, minute)) {  // check if the current time is within this time period, if so return this clock face
                return clock_faces.get_clock_face(index);
            }   // otherwise, loop again until we find it
        }
    }

    return nullptr; // return nullptr if none found, configurations must cover all times of day
}

// this is where we load all the data from the json object (matrix_config.json is currently hardcoded, may change later)
// matrix_config.json should be in the same folder as the executable, when you clone the git repository this should already be satisfied
matrix_clock::clock_face_container load_clock_faces(string config_file) {
    Json::Value jsonData;   // full json from the config file
    JSONCPP_STRING error;
    ifstream file_stream(config_file); // grab the matrix_config file

    Json::CharReaderBuilder builder;    // json value reader

    if (!parseFromStream(builder, file_stream, &jsonData, &error)) {
        cout << "Invalid config file provided." << endl << error << endl;   // if we could not load data, tell the user its invalid
    }                                                                       // the main method will kill the program if the clock face container is empty

    file_stream.close();

    matrix_clock::clock_face_container container;   // create the container and begin loading data

    for (Json::Value::ArrayIndex face_index = 0; face_index != jsonData["clock_faces"].size(); face_index++) {  // loop through ALL clock face declared in the file
        matrix_clock::clock_face *config_clock_face = new matrix_clock::clock_face(jsonData["clock_faces"][face_index]["name"].asString()); // create a new clock face at the current index

        // loop through ALL time periods within the current interface
        for (Json::Value::ArrayIndex times_index = 0; times_index != jsonData["clock_faces"][face_index]["time_periods"].size(); times_index++) {
            Json::Value time_data = jsonData["clock_faces"][face_index]["time_periods"][times_index];

            matrix_clock::time_period clock_face_time_period(time_data["start_hour"].asInt(),   // instantiate an object using json data
                                                             time_data["start_minute"].asInt(),
                                                             time_data["end_hour"].asInt(),
                                                             time_data["end_minute"].asInt());

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
                color = matrix_clock::matrix_color(text_data["color"]["r"].asInt(),
                                                   text_data["color"]["g"].asInt(),
                                                   text_data["color"]["b"].asInt());
            } else {                                                            // a prebuilt color is used, read it in from string
                color = matrix_clock::matrix_color(text_data["color"]["built_in_color"].asString());
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

        container.add_clock_face(config_clock_face);        // add the clock face to the container
    }

    return container;       // return our container of clock faces loaded from json for use in the main loop
}                           // if it ends up being empty from a defective config file, the main loop checks it