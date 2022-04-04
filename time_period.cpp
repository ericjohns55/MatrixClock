// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// time_period.cpp
// Implementation for the time_period class
//

#include "matrix_clock.h"

namespace matrix_clock {
    time_period::time_period(int hour_start, int minute_start, int hour_end, int minute_end) {
        this->hour_start = hour_start;          // populate private field members
        this->minute_start = minute_start;
        this->hour_end = hour_end;
        this->minute_end = minute_end;
    }

    bool time_period::in_time_period(int current_hour, int current_minute) const {
        int time1 = (hour_start * 60) + minute_start;       // convert the start time to the minute of the day
        int time2 = (hour_end * 60) + minute_end + 1440;    // convert the end time to the minute of the day + 1440 in case we cross midnight
        int current_time = (current_hour * 60) + current_minute + 1440;     // convert the current time to the minute of the day + 1440 in case we cross midnight

        int time_difference = (time2 - time1) % 1440;   // check the time difference between start and end and mod out extra day
        int current_difference = (current_time - time1) % 1440; // check the time difference between the current time and the start and mod out extra day

        if (time_difference == current_difference)  // if there is no time difference, catch edge case and say false, a time period cannot take no time
            return false;

        return time_difference > current_difference;    // otherwise, return if we are in the time frame or not
    }
}