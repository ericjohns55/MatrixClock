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
#include "matrix_telegram.h"
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
void update_clock(rgb_matrix::FrameCanvas* offscreen, matrix_clock::clock_face* clock_face, matrix_clock::variable_utility* util, std::string font_folder) {
    offscreen->Clear(); // clear offscreen because it was previously swapped

    matrix_clock::matrix_color bg_color = clock_face->get_background_color();
    offscreen->Fill(bg_color.get_red(), bg_color.get_green(), bg_color.get_blue());

    for (int i = 0; i < clock_face->get_line_count(); i++) {    // loop through all lines to render
        matrix_clock::text_line current_line = clock_face->get_line(i);  // grab current line from the clock face
        current_line.parse_variables(util);     // parse the variables into actual data

        rgb_matrix::Font font; // convert our font to the font declared in the matrix_library
        font.LoadFont(matrix_clock::matrix_font::get_font_file(font_folder, current_line.get_font().get_font()).c_str());

        // draw the text using the color, positionings, and matrix_font size declared on the off screen campus
        rgb_matrix::DrawText(offscreen, font, current_line.parse_x(), current_line.get_y(),
                             current_line.get_color(), current_line.get_parsed_text().c_str());
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) { // make sure the minimum amount of arguments were provided for the program to run
        cerr << "Only " << argc << " arguments provided:" << endl;
        cerr << "Usage: " << argv[0] << " --CONFIG <config file location>" << endl;
        return EXIT_FAILURE;
    }

    string config_file;     // we are going to load both the config file path and the weather url the command arguments

    for (int i = 1; i < argc; i++) {    // loop through all the given arguments
        if (string(argv[i]) == "--CONFIG") {           // check if we found the config file specifier
            if (i + 1 < argc) {                                // make sure there is at least one more position afterwards
                config_file = argv[++i];                       // load the config file name from arguments
            } else {
                cerr << "--CONFIG requires an argument" << endl;   // otherwise warn the user
            }
        }
    }

    if (config_file.empty())    // if either file is empty, count on the previous error messages saying what is wrong
        return EXIT_FAILURE;    // kill the program

    RGBMatrix::Options defaults;
    defaults.hardware_mapping = "adafruit-hat-pwm";     // these are the most optimized hardware options for my screen
    defaults.rows = 64;                                 // yours may be different
    defaults.cols = 64;
    defaults.pwm_lsb_nanoseconds = 130;
    defaults.brightness = 50;
    defaults.limit_refresh_rate_hz = 300;               // helps it a little more stable

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

    matrix_clock::matrix_data clock_data(config_file);    // create clock data object and load data from the config file

    if (!clock_data.load_clock_data()) {
        cerr << "Killing program, please enter valid JSON data into " << config_file << " and run again." << endl;
        return EXIT_FAILURE;
    }

    if (clock_data.get_clock_face_count() == 0) {  // check if none loaded
        cerr << "No valid clock faces found. Make sure at least one clock face is defined in " << config_file << endl;   // kill the program because we cannot have 0 interfaces
        return EXIT_FAILURE;
    }

    matrix_clock::variable_utility time_util(clock_data.get_weather_url());   // generate a time util

    time_util.poll_date();  // on first run, poll date and weather because they have not been loaded yet
    time_util.poll_weather();

    int times[4];           // declare a times array to frequently update
    time_util.get_time(times);

    // load initial clock face by setting it to the current one in the container
    clock_data.update_clock_face(times[3], times[1]);

    matrix_telegram_integration::matrix_telegram telegram_bot(&clock_data, &time_util);

    // only enable the telegram bot if a valid key is entered
    // otherwise the user SHOULD put in disabled as instructed in the repo
    if (clock_data.get_bot_token() != "disabled") {
        telegram_bot.enable_bot();
    }

    // if we do not find a valid clock face for the given time, we will fill with an empty clock face to display nothing on the screen
    update_clock(offscreen, clock_data.get_current(), &time_util, clock_data.get_fonts_folder());
    offscreen = matrix->SwapOnVSync(offscreen);

    // inform console we are starting so there is at least some feedback in console
    cout << "Starting clock loop..." << endl;

    // declare the previous second
    int previous_second = times[2];

    while (!interrupt_received) { // loop until the program is killed
        time_util.get_time(times);  // update our times variable

        int new_second = times[2];  // check the new second

        // only run the following code if the seconds have changed OR if a clock face has demanded an immediate update
        // otherwise sleep for 0.2 seconds
        if (previous_second != new_second) {
            if (!clock_data.check_recent_reload()) {   // we want to skip an extra second if the config was recently reloaded, more information in header file
                bool new_minute = times[2] == 0;    // create boolean for if the minute changed
                previous_second = new_second;       // update previous second for next loop

                if (new_minute) {            // specific tasks that happen every minute
                    if (time_util.is_new_day())     // if the day has changed, poll the new date data (date cannot change on a second)
                        time_util.poll_date();

                    if (times[1] % 5 == 0)   // if the minute is a multiple of 5, update weather info (weather API has a free polling limit, so i only update once every 5 minutes)
                        time_util.poll_weather();

                    if (!clock_data.clock_face_overridden())   // grab the interface again in case it changed as long as the face is not currently overridden (interfaces cannot change on a second)
                        clock_data.update_clock_face(times[3], times[1]);

                    if (clock_data.get_bot_token() != "disabled")   // as long as the bot is active, check to see if we need to send a push notification and do so if one is found
                        telegram_bot.check_send_notifications(times[3], times[1], time_util.get_day_of_week());
                }

                // update only if:
                //      1) we have a second count displayed on the screen that must update every second
                //      2) there is no second count, BUT it is a new minute so we have to update anyways
                //      3) there is a forced update
                // do not update under ANY OTHER CIRCUMSTANCES
                // in terms of the forced update above, this should not run a second time in the same loop unless it is somehow pressed at a new minute
                if (clock_data.get_current()->contains_second_variable() || (!clock_data.get_current()->contains_second_variable() && new_minute) || clock_data.update_required()) {
                    if (clock_data.is_clock_on()) {    // we check this here because we still want to update the interfaces and weather so it is accurate if the clock was off and turned back on
                        update_clock(offscreen, clock_data.get_current(), &time_util, clock_data.get_fonts_folder()); // update only if the clock is on
                        offscreen = matrix->SwapOnVSync(offscreen);
                    }

                    if (clock_data.update_required()) {  // if there is a required update, set it to false so we do not force update again on new second
                        clock_data.set_update_required(false);

                        if (!clock_data.is_clock_on()) {   // clear the screen if it was just turned off
                            offscreen->Clear();
                            offscreen = matrix->SwapOnVSync(offscreen);
                        }
                    }
                }
            } else {
                clock_data.set_recent_reload(false);   // disable the recent reload and grab the current clock face again (since it was just cleared)
                clock_data.update_clock_face(times[3], times[1]);
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