// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// matrix_timer.cpp
// Implementation of the matrix_timer class
//

#include "matrix_clock.h"
#include <iostream>
#include <sstream>

namespace matrix_clock {
    void matrix_timer::calculate_current_time(void) {
        hour = (int) tick_num / 3600;   // tick nums are calculated using (3600 * hour) + (60 * minute) + second
        minute = (int) ((tick_num / 60) % 60);      // do opposite calculation to get the hour minute and second number
        second = tick_num % 60;
    }

    matrix_timer::matrix_timer(int hour, int minute, int second) {
        if (hour != -2 && minute != -2 && second != -2) {       // making sure this is NOT a stopwatch when created
            this->hour = original_hour = hour;      // hold original values for the reset value
            this->minute = original_minute = minute;
            this->second = original_second = second;
            tick_num = (hour * 3600) + (minute * 60) + second;  // calculate how many seconds total are in the timer
        } else {
            original_hour = original_minute = original_second = 0;
            stopwatch = true;       // it is a stopwatch, set this to true so we count up instead of down later
            tick_num = 0;
        }

        hold_ending = 0;        // timer has not ended, make sure hold is 0
    }

    int matrix_timer::tick(int hold_max) {
        if (paused)
            return tick_num; // do not tick a paused timer

        if (started) {  // make sure we only tick the timer if it has been started
            if (!stopwatch) {
                if (tick_num > 0) {     // while the timer has ticks left, subtract 1
                    tick_num--;
                } else {
                    hold_ending++;  // timer has ended, add to the hold ended for how long to keep it on screen
                }
            } else {
                tick_num++; // it is a stopwatch, count up one
            }
        }

        if (hold_ending >= hold_max) {
            end_timer();    // end the timer if we exceed the holding period
            return hold_ending;   // the hold period just ended, return false as well
        }

        // return a negative hold ending if we are in the hold ending period to specify that
        // otherwise return tick_num to show how far we are in the timer
        return (hold_ending > 0) ? hold_ending * -1 : tick_num;
    }

    bool matrix_timer::can_tick(int hold_max) const {
        return (hold_ending + 1) < hold_max || (!started && hour != -1);    // the timer can tick if it is NOT at the end of the hold period
    }                                                                       // or it is not started and the hour isnt -1 (at this point we display it on the screen as non started)

    std::string matrix_timer::format_timer() {
        calculate_current_time();   // calculate the tick time for display

        std::stringstream stream;

        if (hour != 0) {
            stream << hour << ":";  // format the hour into there if it exists

            if (minute < 10) {  // if the minute is less than 10, format with another 0
                stream << "0";
            }
        }

        if (stopwatch && minute < 10 && hour == 0) {    // if we have a timer, make the minutes period two wide for consistency
            stream << "0";
        }

        stream << minute << ":";

        if (second < 10) {
            stream << "0";  // add a 0 for seconds less than 10
        }

        stream << second;

        if (!started && stopwatch) {    // if it is a stopwatch that hasnt started, use 00:00 as a default screen value
            return "00:00";
        }

        return stream.str();
    }

    void matrix_timer::end_timer(void) {
        hour = -1;      // hour being -1 is the ended flag for a timer
        minute = second = 0;
        started = false;
        hold_ending = INT16_MAX;        // this ensures that whatever inputted hold ending is over
    }

    void matrix_timer::reset_timer(void) {
        started = false;        // stop the timer so it does not immediately tick
        hold_ending = 0;
        hour = original_hour;       // load original given values
        minute = original_minute;
        second = original_second;

        if (stopwatch) {
            original_hour = original_minute = original_second = 0;
            tick_num = 0;   // if it was a stopwatch, set the tick num back to the default 0
        }
    }
}