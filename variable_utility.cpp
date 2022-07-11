// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// variable_utility.cpp
// Implementation for the variable_utility class
//

#include <ctime>
#include <cmath>
#include <memory>
#include <sstream>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include "matrix_clock.h"

namespace matrix_clock {
    // callback method for curl command
    std::size_t callback(const char*, std::size_t, std::size_t, std::string*);

    // variable_utility(std::string url)
    //      constructor to create the variable utility object
    //      also loads the default placeholder values for the weather
    //          if on run the application cannot find any weather, it will display ~Error~ for the forecast and 0s for all data
    //          if the weather cannot be updated otherwise, it will display the most recent information
    //
    //      url = an OpenWeatherAPI weather URL to load data from
    variable_utility::variable_utility(std::string url) {
        weather_url = url;

        forecast = "N/A";        // default placeholder values, will be shown in case the weather cannot be updated
        short_forecast = "~Error~";
        temp = 0;
        real_feel = 0;
        wind_speed = 0.0;
        humidity = 0;
}

    // replace_string(std::string& source, std::string search, std::string replace)
    //      replace all instances of "search" in the source string with "replace"
    //
    //      source = the string to execute a replace on
    //      search = the string we want to have replaced
    //      replace = what we want to the search term with
    void replace_string(std::string& source, std::string search, std::string replace);

    // pad_numbers(int source)
    //      pad the source number to be two digits with a leading 0
    //      this exists mostly for standard readability of the minutes for the clock
    //
    //      source = the integer to add a leading 0 in front of (if necessary, if the source is greater than 10 than it does nothing but convert to string)
    std::string pad_numbers(int source);

    void variable_utility::poll_weather() {
        CURL* curl = curl_easy_init();  // initialize curl
        curl_easy_setopt(curl, CURLOPT_URL, weather_url.c_str()); // load url
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);  // set timeout

        long httpCode(0);   // response code
        std::unique_ptr<std::string> httpData(new std::string()); // data returned

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());

        curl_easy_perform(curl);       // perform command
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode); // load response code
        curl_easy_cleanup(curl);    // cleanup

        if (httpCode == 200) { // successful code
            Json::Value jsonData;   // all data
            JSONCPP_STRING error;

            Json::CharReaderBuilder builder;
            const std::unique_ptr<Json::CharReader> reader(builder.newCharReader()); // json reader object

            const char* httpDataString = httpData.get()->c_str();

            // try parse
            if (reader->parse(httpDataString, httpDataString + httpData->size(), &jsonData, &error)) {
                forecast = jsonData["weather"][0]["description"].asString();        // load weather info from json
                short_forecast = jsonData["weather"][0]["main"].asString();
                temp = round(jsonData["main"]["temp"].asFloat());
                real_feel = round(jsonData["main"]["feels_like"].asFloat());
                wind_speed = round(jsonData["wind"]["speed"].asFloat() * 10) / 10.0;
                humidity = round(jsonData["main"]["humidity"].asFloat());

                if (short_forecast.find("Thunder") != std::string::npos) {    // edge case because Thunderstorms does not fit on matrix
                    short_forecast = "T-Storms";
                }
            } else { // could not parse, something went wrong
                std::cout << "Could not parse data as JSON." << std::endl << error << std::endl;
            }
        } else { // update unsuccessful, display in console
            std::cout << "Could not update weather." << std::endl;
        }
    }

    void variable_utility::poll_date() {
        tm* time_struct = get_tm(); // get

        day_of_month = time_struct->tm_mday;        // load field variables from time struct methods
        day_of_week = time_struct->tm_wday;
        year = time_struct->tm_year + 1900;
        month_num = time_struct->tm_mon + 1;
        month_name = months[time_struct->tm_mon];
        day_name = days[time_struct->tm_wday];

        std::stringstream sstream;  // string builder to create the formatted date variable
        sstream << month_num << "-" << day_of_month << "-" << year;
        formatted_date = sstream.str();
    }

    std::string variable_utility::parse_variables(std::string vars) {
        std::string parsed_text(vars);

        int times[4];       // load times from data object
        get_time(times);

        // perform text replacements using the parsed text field and defined variables
        // fix formatting where necessary (padding 0s and converting time to am or pm)
        replace_string(parsed_text, "{hour}", std::to_string(times[0]));
        replace_string(parsed_text, "{minute}", pad_numbers(times[1]));
        replace_string(parsed_text, "{second}", pad_numbers(times[2]));
        replace_string(parsed_text, "{hour24}", std::to_string(times[3]));
        replace_string(parsed_text, "{ampm}", times[3] < 12 ? "am" : "pm");
        replace_string(parsed_text, "{temp}", std::to_string(get_temp()));
        replace_string(parsed_text, "{temp_feel}", std::to_string(get_real_feel()));
        replace_string(parsed_text, "{humidity}", std::to_string(get_humidity()));
        replace_string(parsed_text, "{forecast}", get_forecast());
        replace_string(parsed_text, "{forecast_short}", get_forecast_short());
        replace_string(parsed_text, "{date_format}", get_formatted_date());
        replace_string(parsed_text, "{month_name}", get_month_name());
        replace_string(parsed_text, "{day_name}", get_day_name());
        replace_string(parsed_text, "{month_num}", std::to_string(get_month_num()));
        replace_string(parsed_text, "{month_day}", std::to_string(get_day_of_month()));
        replace_string(parsed_text, "{week_day_num}", std::to_string(get_day_of_week()));
        replace_string(parsed_text, "{year}", std::to_string(get_year()));

        // for wind speed, we are truncating to 1 decimal place for easier readability (nobody cares how exact it is)
        std::string wind = std::to_string(get_wind_speed());
        replace_string(parsed_text, "{wind_speed}", wind.substr(0, wind.find(".") + 2));

        return parsed_text;
    }

    std::tm* variable_utility::get_tm() {
        time_t now = std::time(0);  // generate a date and time struct with current time
        tm* time = localtime(&now);
        return time;    // return struct
    }

    bool variable_utility::is_new_day() {
        int times[4];   // grab times from the current struct
        get_time(times);

        return times[3] == 0 && times[1] == 0 && times[2] == 0; // check if 24 hour time is 0, minutes is 0, and seconds is 0
    }

    void variable_utility::get_time(int times[]) {
        tm* time_components = get_tm(); // grab current time struct and load array with time data
        times[0] = time_components->tm_hour % 12;  // hour in 12 format
        times[1] = time_components->tm_min;         // minute
        times[2] = time_components->tm_sec;         // second
        times[3] = time_components->tm_hour;        // 24-hour format

        if (times[0] == 0) // non-military time
            times[0] = 12;  // make it 12 (standard)
    }

    // std::size_t callback(const char*, std::size_t, std::size_t, std::string*)
    //     callback method for the curl library, this is where it reads the data from the url and writes it to our object
    std::size_t callback(const char* in, std::size_t size, std::size_t num, std::string* out) {
        const std::size_t totalBytes(size * num);
        out->append(in, totalBytes);    // load data from the webpage and send it back out
        return totalBytes;
    }

    std::string pad_numbers(int source) {
        std::stringstream stream;

        if (source < 10) {
            stream << "0";  // if the integer is less than 10, add in a zero infront for readability in time
        }

        stream << std::to_string(source);

        return stream.str();
    }

    void replace_string(std::string& source, std::string search, std::string replace) {
        size_t position = source.find(search);  // find the current index of the search string

        while (position != std::string::npos) { // loop until it cannot be found anymore (replaced out), will not run with 0 instances
            source.replace(position, search.size(), replace);   // replace at the position
            position = source.find(search, position + replace.size());  // find next instance (will not loop again with none found)
        }
    }
}