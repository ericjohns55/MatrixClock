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

#include "matrix_clock.h"

#include "led-matrix.h"
#include "graphics.h"

using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

using namespace std;

// boolean that checks if there is an interrupt pushed to the system
volatile bool interrupt_received = false;

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
    if (argc < 7) { // make sure the minimum amount of arguments were provided for the program to run
        cerr << "Only " << argc << " arguments provided:" << endl;
        cerr << "Usage: " << argv[0] << " --WEATHER_URL <weather url> --CONFIG_FILE <config file location> --TELEGRAM_API <bot api key>" << endl;
        return EXIT_FAILURE;
    }

    string config_file;     // we are going to load both the config file path and the weather url the command arguments
    string weather_url;
    string api_key;

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
        } else if (string(argv[i]) == "--TELEGRAM_API") {
            if (i + 1 < argc) {
                api_key = argv[++i];
            } else {
                cerr << "--TELEGRAM_API requires an argument" << endl;
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

    matrix_clock::clock_face_container clock_faces(config_file);    // create clock face container and load data from the config file
    clock_faces.load_clock_faces();

    if (clock_faces.get_clock_face_count() == 0) {  // check if none loaded
        cerr << "Invalid configuration file provided." << endl;   // kill the program because we cannot have 0 interfaces
        return EXIT_FAILURE;
    }

    matrix_clock::variable_utility time_util(weather_url);   // generate a time util

    time_util.poll_date();  // on first run, poll date and weather because they have not been loaded yet
    time_util.poll_weather();

    int times[4];           // declare a times array to frequently update
    time_util.get_time(times);

    // load initial clock face by setting it to the current one in the container
    clock_faces.update_clock_face(times[3], times[1]);

    // load and enable telegram bot
    matrix_clock::matrix_telegram test(&clock_faces, &time_util, api_key);
    test.enable_bot();

    // if we do not find a valid clock face for the given time, we will fill with an empty clock face to display nothing on the screen
    update_clock(offscreen, clock_faces.get_current(), &time_util);
    offscreen = matrix->SwapOnVSync(offscreen);

    // declare the previous second
    int previous_second = times[2];

    while (!interrupt_received) { // loop until the program is killed
        time_util.get_time(times);  // update our times variable

        int new_second = times[2];  // check the new second

        // only run the following code if the seconds have changed OR if a clock face has demanded an immediate update
        // otherwise sleep for 0.2 seconds
        if (previous_second != new_second) {
            if (!clock_faces.check_recent_reload()) {   // we want to skip an extra second if the config was recently reloaded, more information in header file
                bool new_minute = times[2] == 0;    // create boolean for if the minute changed
                previous_second = new_second;       // update previous second for next loop

                if (time_util.is_new_day())         // if the day has changed, poll the new date data
                    time_util.poll_date();

                if (times[1] % 5 == 0 && new_minute)    // if the minute is a multiple of 5, update weather info (weather API has a free polling limit, so i only update once every 5 minutes)
                    time_util.poll_weather();

                if (new_minute && !clock_faces.clock_face_overridden()) {   // if there is a new minute, grab the interface again in case it changed (interfaces cannot change on a second)
                    clock_faces.update_clock_face(times[3], times[1]);
                }

                // update only if:
                //      1) we have a second count displayed on the screen that must update every second
                //      2) there is no second count, BUT it is a new minute so we have to update anyways
                //      3) there is a forced update
                // do not update under ANY OTHER CIRCUMSTANCES
                // in terms of the forced update above, this should not run a second time in the same loop unless it is somehow pressed at a new minute
                if (clock_faces.get_current()->contains_second_variable() || (!clock_faces.get_current()->contains_second_variable() && new_minute) || clock_faces.update_required()) {
                    if (clock_faces.is_clock_on()) {    // we check this here because we still want to update the interfaces and weather so it is accurate if the clock was off and turned back on
                        update_clock(offscreen, clock_faces.get_current(), &time_util); // update only if the clock is on
                        offscreen = matrix->SwapOnVSync(offscreen);
                    }

                    if (clock_faces.update_required()) {  // if there is a required update, set it to false so we do not force update again on new second
                        clock_faces.set_update_required(false);

                        if (!clock_faces.is_clock_on()) {   // clear the screen if it was just turned off
                            offscreen->Clear();
                            offscreen = matrix->SwapOnVSync(offscreen);
                        }
                    }
                }
            } else {
                clock_faces.set_recent_reload(false);   // disable the recent reload and grab the current clock face again (since it was just cleared)
                clock_faces.update_clock_face(times[3], times[1]);
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