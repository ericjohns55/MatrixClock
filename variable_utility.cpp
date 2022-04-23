// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// variable_utility.cpp
// Implementation for the variable_utility class
//

#include <ctime>
#include <sstream>
#include <cmath>
#include <memory>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include "matrix_clock.h"

namespace matrix_clock {
    std::size_t callback(const char*, std::size_t, std::size_t, std::string*);  // callback method for curl command

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
        } else { // unsuccessful, let the user know
            std::cout << "Could not update weather, loading placeholder values instead." << std::endl;

            forecast = "N/A";        // load placeholder values so the application can still run
            short_forecast = "~Error~";
            temp = 0;
            real_feel = 0;
            wind_speed = 0.0;
            humidity = 0;
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
}