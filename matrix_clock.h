// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// matrix_clock.h
//      Holds all the class declarations for the matrix_clock namespace
//

#ifndef MATRIXCLOCK_MATRIX_CLOCK_H
#define MATRIXCLOCK_MATRIX_CLOCK_H

#include <string>
#include <vector>
#include <ctime>
#include <cstring>
#include <algorithm>
#include "graphics.h"

namespace matrix_clock {
    // time_period class
    //      Represents a period of time between a start and end time
    class time_period {
        private:
            int hour_start, minute_start, hour_end, minute_end;
        public:
            // constructor (hour_start, minute_start, hour_end, minute_end)
            //   creates a time period object with the specified start and end times
            time_period(int, int, int, int);

            // takes in two integers: current hour and current minute
            //      returns true if the current time is within this time period, false if not
            bool in_time_period(int, int) const;
    };

    // enum for colors hardcoded in this project
    enum matrix_prebuilt_colors {
        red, orange, yellow, green, blue, purple, pink,
        white, gray, black, brown, night_time
    };

    // matrix_color class
    //      Represents a color that can be used to print text in on the matrix display
    class matrix_color {
        private:
            std::map<std::string, matrix_prebuilt_colors> colors{     {"red", matrix_prebuilt_colors::red},
                {"orange", matrix_prebuilt_colors::orange}, {"yellow", matrix_prebuilt_colors::yellow},
                {"green", matrix_prebuilt_colors::green},   {"blue", matrix_prebuilt_colors::blue},
                {"purple", matrix_prebuilt_colors::purple}, {"pink", matrix_prebuilt_colors::pink},
                {"white", matrix_prebuilt_colors::white},   {"gray", matrix_prebuilt_colors::gray},
                {"black", matrix_prebuilt_colors::black},   {"brown", matrix_prebuilt_colors::brown},
                                                                  {"night_time", matrix_prebuilt_colors::night_time} };
            int r = 0, g = 0, b = 0;

            // parses a prebuilt color into the object as rgb values
            void parse_color(matrix_prebuilt_colors);
        public:
            // constructor that takes in 3 values: red, green, and blue
            //      these values should all be between 0 and 255
            matrix_color(int, int, int);

            // default constructor that creates a black color object (all 0's in the rgb values)
            inline matrix_color() { r = g = b = 0; }

            // constructor that takes in a prebuilt color and parses it into the object
            inline matrix_color(matrix_prebuilt_colors hardcoded_color) { parse_color(hardcoded_color); }

            // constructor that takes in a color name as a string, and parses it into the class
            //      the color should be one of the prebuilt colors
            //      this constructor will be used most when getting data from json
            inline matrix_color(std::string color_name) { parse_color(colors[color_name]); }

            // getter for the red value of the color
            inline int get_red(void) const { return r; }

            // getter for the green value of the color
            inline int get_green(void) const { return g; }

            // getter for the blue value of the color
            inline int get_blue(void) const { return b; }
    };

    // enum for font sizes
    enum matrix_font { small, medium, large, large_bold };

    // font class
    //      Helper methods to parse a matrix_font enum into a font usable on the matrix display
    class font {
        public:
            // get the width of one of the hardcoded matrix_font's
            static int get_x(matrix_font);

            // return the font file location for one of the matrix_font values
            static std::string parse_font(matrix_font);

            // get a matrix_font object for a string entered
            // this string should have the same value as a matrix_font enum name
            // this will be used more when parsing values from json
            static matrix_font font_from_string(std::string);
    };

    // variable_utility class
    //      A helper class that reads weather data from the web and time/date information from the system
    class variable_utility {
        private:
            std::string weather_url;
            int temp, real_feel, humidity;
            float wind_speed;
            std::string short_forecast, forecast;
            std::string formatted_date, month_name, day_name;
            int month_num, day_of_month, day_of_week, year;
            // time variables not necessary

            // returns the time construct that helps us gather date and time data
            std::tm* get_tm();

            const std::string months[12] {"January", "February", "March", "April",
                                          "May", "June", "July", "August", "September",
                                          "October", "November", "December"};
            const std::string days[7] {"Sunday", "Monday", "Tuesday", "Wednesday",
                                       "Thursday", "Friday", "Saturday"};
        public:
            // constructor that requires a weather URL from OpenWeatherMap as a string to parse weather data with
            inline variable_utility(std::string url) { weather_url = url; }

            // polls the weather URL and updates the weather fields with the new data
            void poll_weather(void);

            // polls the system for new date information and updates the date field with the new data
            void poll_date(void);

            // replace all valid variables in the string parameter into a new string and return it
            std::string parse_variables(std::string vars);

            // returns true if the time is exactly midnight (and on the first second), false if not
            bool is_new_day(void);

            // loads time data into an array of length 4
            //      index 0: 12 hour time
            //      index 1: minute (1-60)
            //      index 2: second (1-60)
            //      index 3: 24 hour time
            void get_time(int times[]);

            // returns the current temperature
            //      poll_weather() must be called before this is usable
            inline int get_temp(void) const { return temp; }

            // returns the current real feel
            //      poll_weather() must be called before this is usable
            inline int get_real_feel(void) const { return real_feel; }

            // returns the current humidity
            //      poll_weather() must be called before this is usable
            inline int get_humidity(void) const { return humidity; }

            // returns the current wind speed rounded to one decimal place
            //      poll_weather() must be called before this is usable
            inline float get_wind_speed(void) const { return wind_speed; }

            // returns the current forecast
            //      poll_weather() must be called before this is usable
            //      this string will typically be too long to fit on the matrix but is included
            //      in case someone wants to parse it further and wrap around
            inline std::string get_forecast(void) const { return forecast; }

            // returns the current weather outside as one word
            //      poll_weather() must be called before this is usable
            inline std::string get_forecast_short(void) const { return short_forecast; }

            // returns the date formatted as MM-DD-YYYY
            //      poll_date() must be called before this is usable
            inline std::string get_formatted_date(void) const { return formatted_date; }

            // returns the name of the current month
            //      poll_date() must be called before this is usable
            inline std::string get_month_name(void) const { return month_name; }

            // returns the name of the current day
            //      poll_date() must be called before this is usable
            inline std::string get_day_name(void) const { return day_name; }

            // returns what number month it is
            //      poll_date() must be called before this is usable
            //      month 1 is January, 2 is February, etc
            inline int get_month_num(void) const { return month_num; }

            // returns the current day of the month
            //      poll_date() must be called before this is usable
            //      example: June 23rd would return 23
            inline int get_day_of_month(void) const { return day_of_month; }

            // returns current day of the week [0-6]
            //      poll_date() must be called before this is usable
            //      Sunday is considered 0, Monday 1, Tuesday 2, etc
            inline int get_day_of_week(void) const { return day_of_week; }

            // returns the current year
            //      poll_date() must be called before this is usable
            inline int get_year(void) const { return year; }

            // manually set the weather URL
            // only use this after reloading clock data via a telegram bot
            // the load_clock_data() method automatically loads the weather URL from the config file
            //          on first run into the variable_utility object
            inline void set_weather_url(std::string url) { weather_url = url; }
    };

    // text_line class
    //      Represents a line of text that can be shown on the matrix
    class text_line {
        private:
            matrix_color color;
            matrix_font font_size;
            int x_pos;
            int y_pos;
            std::string text;
            std::string parsed_text;
        public:
            // constructor that takes in a color, font, x position, y position, and a text string
            //      instantiates the text_line object using these values
            text_line(matrix_color, matrix_font, int, int, std::string);

            // parse all variables passed into the text object as actual dadta
            // use the variable_utility to convert these variables into readable data
            void parse_variables(matrix_clock::variable_utility* util);

            // get the current x position of the variable
            //      if the x position is -1, then it will return an x value that will center the text line on the screen
            //      if the position is NOT -1, then it will return whatever x value was passed in
            int parse_x(void);

            // get the current color converted to a color that the matrix library can use to draw text
            const rgb_matrix::Color get_color(void) const;

            // return the text that has all the variables replaced to readable data
            // it is assumed that you will call parse_variables() before this
            inline std::string get_parsed_text(void) { return parsed_text; }

            // get the font specified for the text line
            inline matrix_font get_font(void) const { return font_size; }

            // get the y positioning of the text line
            // the y positioning is considered the bottom of the line of text
            inline int get_y(void) { return y_pos; }
    };

    // clock_face class
    //      Represents all the visible information of the matrix
    //      Contains all the lines of text and the time periods it is visible
    class clock_face {
        private:
            std::string name;
            matrix_color background_color;
            std::vector<text_line> text_lines;
            std::vector<time_period> time_periods;
            bool contains_seconds_code;
        public:
            // instantiates a clock face with a specified name
            inline clock_face(std::string name, matrix_color bg_color) { this->name = name; contains_seconds_code = false;
                background_color = bg_color; }

            // adds a time period to the clock face
            // (a clock face can contain many time periods)
            inline void add_time_period(time_period period) { time_periods.push_back(period); }

            // returns a vector of all the time periods
            inline std::vector<time_period> get_time_periods(void) const { return time_periods; }

            // adds a new line of text to the matrix as a text_line object
            // (a clock can contain many lines of text)
            inline void add_text(text_line text) { text_lines.push_back(text); }

            // get a text_line object contained by the "line" index of the text_line on the matrix
            inline text_line get_line(int line) { return text_lines.data()[line]; }

            // get the total amount of text_lines contained in the clock face object
            inline int get_line_count(void) { return text_lines.size(); }

            // gets the name of the interface
            // this will be relevant more later when telegram bot implementation is added
            inline std::string get_name(void) const { return name; }

            // get the background color of the clock face
            inline matrix_color get_background_color(void) const { return background_color; }

            // set if the clock face contains a {second} variable or not
            // description of why this is important under contains_second_variable()
            inline void set_contains_second_variable(bool contained) { contains_seconds_code = contained; }

            // return whether the clock has a second variable
            // this is important because if there is no {second} variable, then we only
            // need to update the clock every minute
            // if there is a second variable, then we know we need to update the clock every second
            inline bool contains_second_variable(void) const { return contains_seconds_code; }
    };

    // telegram_push class
    //      Represents the data that would be used for a scheduled push notification
    class telegram_push {
    private:
        std::string message;
        int hour, minute;
        std::vector<int> days;
    public:
        // default constructor, takes in all data for the field variables (it is assumed this will be used while parsing the config json file)
        telegram_push(std::string message, int hour, int minute, std::vector<int> days) {
            this->message = message;        this->hour = hour;      this->minute = minute;      this->days = days;
        }

        // checks if it is time to send a push notification, returns true if so
        // if hour is -1, we consider it hourly and this is acceptable
        inline bool is_push_time(int current_hour, int current_minute, int day_of_week) {
            return (current_hour == hour || hour == -1) && current_minute == minute && std::find(days.begin(), days.end(), day_of_week) != days.end();
        }

        // grab the message for the notification
        inline std::string get_message(void) const { return message; }
    };

    // matrix_data class
    //      Represents a container of clock faces to hold everything needed for the matrix
    class matrix_data {
        private:
            std::vector<clock_face*> clock_faces;
            std::vector<telegram_push*> push_notifications;
            clock_face* current;
            clock_face* empty;
            std::string config_file;
            std::string weather_url;
            std::string bot_token;
            std::int64_t bot_chat_id;
            bool config_recently_reloaded;
            bool override_interface;
            bool force_update;
            bool clock_on;
        public:
            // default constructor, instantiates an empty container
            matrix_data(std::string config_file);

            // add a new clock_face pointer to the container
            inline void add_clock_face(clock_face* new_clock_face) { clock_faces.push_back(new_clock_face); }

            // add a new push notification pointer to the container
            inline void add_notification(telegram_push* notification) { push_notifications.push_back(notification); }

            // return the amount of clock faces in the container
            inline size_t get_clock_face_count(void) const { return clock_faces.size(); }

            // get the names of all the clock faces as a string array
            std::string* get_names(void) const;

            // update the clock face to one with the given name
            void update_clock_face(std::string name);

            // update the clock face in the given time
            void update_clock_face(int hour, int minute);

            //this loads all the clock faces and other data from the matrix_config.json in the repository
            //you can edit matrix_config.json all you want, as long as valid json and data is submitted than it will load the
            //program using that data and update everything when needed
            //this MUST BE called before you attempt to write any data to the screen
            //otherwise nothing will be loaded and there will be no information to grab for writing
            bool load_clock_data();

            // get the current clock face
            inline clock_face* get_current(void) const { return current; }

            // get the weather URL declared in matrix_config.json
            // Note: you MUST run load_clock_data() before this is valid
            inline std::string get_weather_url(void) const { return weather_url; }

            // get the bot token declared in matrix_config.json
            // Note: you MUST run load_clock_data() before this is valid
            inline std::string get_bot_token(void) const { return bot_token; }

            // get the defined chat id
            // Note: you MUST run load_clock_data() before this is valid
            inline std::int64_t get_chat_id(void) const { return bot_chat_id; }

            // get the telegram push notifications vector
            inline std::vector<telegram_push*> get_notifications(void) const { return push_notifications; }

            // check whether the clock face is overridden via the telegram bot
            inline bool clock_face_overridden(void) const { return override_interface; }

            // set whether the clock face is overridden
            inline void set_clock_face_override(bool override) { override_interface = override; }

            // check if we need to force a clock face update
            inline bool update_required(void) const { return force_update; }

            // set the need for a forced clock face update
            inline void set_update_required(bool required) { force_update = required; }

            // check if the clock is on or not
            inline bool is_clock_on(void) const { return clock_on; }

            // set the clock on (true) or off (false)
            inline void set_clock_on(bool on) { clock_on = on; }

            // check whether the config was recently reloaded
            // we want to check this because the telegram bot runs on a separate thread so it is possible
            // that a natural update could run while we are clearing it
            // in this scenario we want to make sure it is not in the process of being reloaded while we are accessesing
            // data because it could potentially become null halfway through updating
            inline bool check_recent_reload(void) const { return config_recently_reloaded; }

            // check whether the config was recently reloaded
            inline void set_recent_reload(bool reloaded)  { config_recently_reloaded = reloaded; }
    };
}

#endif //MATRIXCLOCK_MATRIX_CLOCK_H