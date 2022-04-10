// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// telegram_handler.cpp
// The handler for commands and the inline buttons for the telegram controller of the bot
//

#include <string>
#include <thread>
#include <tgbot/tgbot.h>
#include "matrix_clock.h"

namespace matrix_clock {
    // bot_handler(std::string api_key, matrix_clock::clock_face_container* container, matrix_clock::variable_utility* var_util);
    //      Contains all the commands and callback functions for the bot
    //      Has the /buttons command to generate the Inline Keyboard, and the callback function for the button presses
    //
    //      api_key = the api key for the telegram bot (required to run)
    //      container = the clock face container that contains all valid clock faces
    //      var_util = the var util used throughout the program to poll for new data
    void bot_handler(std::string api_key, matrix_clock::clock_face_container* container, matrix_clock::variable_utility* var_util);

    // get_names(matrix_clock::clock_face_container* container)
    //      Returns an array of all the names of the clock faces in the configuration file
    //          - the array length is equal to the clock faces count [container->get_clock_face_count()]
    //      Formats them to be all lowercase except the first letter
    std::string* get_names(matrix_clock::clock_face_container* container);


    matrix_telegram::matrix_telegram(matrix_clock::clock_face_container* container, matrix_clock::variable_utility* var_util, std::string api) {
        clock_face_container = container;   // load required pointers and the API key for manipulation by the bot
        util = var_util;
        api_key = api;
    }

    void matrix_telegram::enable_bot() {
        std::thread poll_bot(bot_handler, api_key, clock_face_container, util); // starts the bot in a separate thread
        poll_bot.detach();  // detach so the thread does not die when we leave the method scope
    }

    void bot_handler(std::string api_key, matrix_clock::clock_face_container* container, matrix_clock::variable_utility* var_util) {
        TgBot::Bot bot(api_key);    // create a new bot using the API Key

        TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup); // the inline keyboard for the clock faces

        std::string* name_array = get_names(container); // grab the names of all the faces to load into the clock face

        // generate inline keyboard button command
        bot.getEvents().onCommand("buttons", [&bot, &keyboard, &container, &name_array](TgBot::Message::Ptr message) {
            int length = container->get_clock_face_count(); // length of the clock faces container

            // this is the loop for the amount of rows we will have, three clock faces names allowed per row
            for (size_t i = 0; i < (length / 3) + (length % 3 == 0 ? 0 : 1); i++) { // loop until we have the amount of rows = to the count / 3 + 1 more if there is extra
                std::vector<TgBot::InlineKeyboardButton::Ptr> row;  // vector of buttons for the cell

                for (size_t j = 0; j < 3; j++) {    // loop until there is three buttons
                    int location = j + (i * 3); // add 3 * the row count to convert to the position in the array

                    if (location == length) // break if we passed the last button
                        break;

                    TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);   // create a new button
                    button->text = name_array[location];        // set the button text and callback data to the clock face name
                    button->callbackData = name_array[location];

                    row.push_back(button);  // push thee button to the row
                }

                keyboard->inlineKeyboard.push_back(row);    // push the row to the keyboard
            }

            // send the keyboard to the user
            bot.getApi().sendMessage(message->chat->id, "Clock Interfaces", false, 0, keyboard, "Markdown");
        });

        // callback query to the inline keyboard
        bot.getEvents().onCallbackQuery([&container](TgBot::CallbackQuery::Ptr query) {
            if (!StringTools::startsWith(query->data, "command")) { // make sure it doesnt start with command, there are other buttons
                container->set_current(container->get_clock_face(query->data)); // set the current clock face to the name pressed
                container->set_clock_face_override(true);       // set the clock face override on, this way it will stay and not change with time
                container->set_update_required(true);       // force clock update now
            } else {
                // TODO: commands
            }
        });

        TgBot::TgLongPoll long_poll(bot); // this starts the poll

        while (true) {      // loop forever (this runs again on loop)
            long_poll.start();  // run the poll again after next data
        }
    }

    std::string* get_names(matrix_clock::clock_face_container* container) {
        std::string* names_array = new std::string[container->get_clock_face_count()];  // create a new array with a length of the clock face count

        for (size_t i = 0; i < container->get_clock_face_count(); i++) {    // iterate through each clock face
            std::string name = container->get_clock_face(i)->get_name();    // grab the name of the clock face

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