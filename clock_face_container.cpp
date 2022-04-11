// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// clock_face_container.cpp
// Implementation of the clock_face_container class
//

#include "matrix_clock.h"

namespace matrix_clock {
    clock_face_container::clock_face_container() {
        empty = new clock_face("~empty~");  // create an empty clock face in the background
        override_interface = false;
        force_update = false;
        clock_on = true;
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
}

