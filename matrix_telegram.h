//
// Created by Eric on 4/23/2022.
//

#ifndef MATRIXCLOCK_MATRIX_TELEGRAM_H
#define MATRIXCLOCK_MATRIX_TELEGRAM_H

#include <tgbot/tgbot.h>
#include "matrix_clock.h"

namespace matrix_telegram_integration {
    // matrix_telegram class
    //      Launches the telegram integration of the bot
    //      More information about what
    //      the telegram bot does in the README file in this repository
    class matrix_telegram {
        private:
            matrix_clock::matrix_data* matrixData;
            matrix_clock::variable_utility* util;
            std::string api_key;
            std::int64_t chat_id;
            TgBot::Bot* bot;
        public:
            // default constructor, pulls in the clock container and variable utility for use in the bot, and the API key
            matrix_telegram(matrix_clock::matrix_data*, matrix_clock::variable_utility*);

            // enables the callback for the telegram bot, it will not run unless this is called
            void enable_bot();

            // sends a telegram push notification declared in the matrix config
            // hour = current hour; minute = current minute; day_of_week = the current day of week (sunday = 0)
            void check_send_notifications(int hour, int minute, int day_of_week);
    };
}

#endif //MATRIXCLOCK_MATRIX_TELEGRAM_H
