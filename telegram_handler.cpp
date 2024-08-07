// Matrix Clock
// Created by Eric Johns (ericjohns55)
// https://github.com/ericjohns55/MatrixClock
//
// telegram_handler.cpp
// The handler for commands and the inline buttons for the telegram controller of the bot
//

#include <string>
#include <thread>
#include <sstream>
#include <wiringPi.h>
#include "matrix_telegram.h"
#include <iostream>

namespace matrix_telegram_integration {
    // bot_handler(std::string api_key, matrix_clock::matrix_data* container, matrix_clock::variable_utility* var_util);
    //      Contains all the commands and callback functions for the bot
    //      Has the /buttons command to generate the Inline Keyboard, and the callback function for the button presses
    //
    //      api_key = the api key for the telegram bot (required to run)
    //      container = the clock face container that contains all valid clock faces
    //      var_util = the var util used throughout the program to poll for new data
    void bot_handler(TgBot::Bot* bot, matrix_clock::matrix_data* container, matrix_clock::variable_utility* var_util);

    // returns the timer control board for the /timer and /stopwatch commands
    TgBot::InlineKeyboardMarkup::Ptr get_timer_controls(void);

    // sends a telegram message asynchronously to prevent visual stutters in the program
    // these stutters are most visible when you see a seconds variable, calling this method in a thread fixes it
    void push_telegram_separate_thread(std::string message, TgBot::Bot* bot, std::int64_t chat_id, bool dismissable);

    // sends a message to the defined chat_id with an inline keyboard containing
    // a single dismiss button that will delete the message in chat
    void send_dismiss_keyboard(std::string message, TgBot::Bot* bot, std::int64_t chat_id);

    matrix_telegram::matrix_telegram(matrix_clock::matrix_data* data, matrix_clock::variable_utility* var_util) {
        matrixData = data;   // load required pointers and the API key for manipulation by the bot
        util = var_util;
        api_key = matrixData->get_bot_token();
        chat_id = matrixData->get_chat_id();
        bot = new TgBot::Bot(api_key);  // create bot object
    }

    void matrix_telegram::enable_bot() {
        std::thread poll_bot(bot_handler, bot, matrixData, util); // starts the bot in a separate thread
        poll_bot.detach();  // detach so the thread does not die when we leave the method scope
    }

    void matrix_telegram::check_send_notifications(int hour, int minute, int day_of_week) {
        std::vector<matrix_clock::telegram_push*> notifications = matrixData->get_notifications();
        std::vector<matrix_clock::telegram_push*>::iterator iter;

        for (iter = notifications.begin(); iter != notifications.end(); iter++) {
            matrix_clock::telegram_push* current_notification = *iter;

            if (current_notification->is_push_time(hour, minute, day_of_week)) {
                send_dismiss_keyboard(util->parse_variables(current_notification->get_message()), bot, chat_id);
            }
        }
    }

    void bot_handler(TgBot::Bot* bot, matrix_clock::matrix_data* container, matrix_clock::variable_utility* var_util) {
        TgBot::InlineKeyboardMarkup::Ptr clock_faces_keyboard(new TgBot::InlineKeyboardMarkup); // the inline clock_faces_keyboard for the clock faces

        std::string* name_array = container->get_names(); // grab the names of all the faces to load into the clock face

        // generate inline keyboards for the user
        bot->getEvents().onCommand("buttons", [&bot, &clock_faces_keyboard, &container, &name_array](TgBot::Message::Ptr message) {
            // delete the /buttons message (this is for cleanliness in a non group chat (so there are no permission issues))
            if (message->chat->type == TgBot::Chat::Type::Private) {
                bot->getApi().deleteMessage(message->chat->id, message->messageId);
            }

            // GENERATING THREE INLINE KEYBOARDS:
            // FIRST KEYBOARD: clock face override

            int length = container->get_clock_face_count(); // length of the clock faces container

            // this is the loop for the amount of rows we will have, three clock faces names allowed per row
            for (int i = 0; i < (length / 3) + (length % 3 == 0 ? 0 : 1); i++) { // loop until we have the amount of rows = to the count / 3 + 1 more if there is extra
                std::vector<TgBot::InlineKeyboardButton::Ptr> row;  // vector of buttons for the cell

                for (size_t j = 0; j < 3; j++) {    // loop until there is three buttons
                    int location = j + (i * 3); // add 3 * the row count to convert to the position in the array

                    if (location == length) // break if we passed the last button
                        break;

                    TgBot::InlineKeyboardButton::Ptr button(new TgBot::InlineKeyboardButton);   // create a new button
                    button->text = name_array[location];        // set the button text and callback data to the clock face name
                    button->callbackData = name_array[location];

                    row.push_back(button);  // push the button to the row
                }

                clock_faces_keyboard->inlineKeyboard.push_back(row);    // push the row to the clock face keyboard
            }

            std::vector<TgBot::InlineKeyboardButton::Ptr> clear_row;  // row for the clear clock face override button

            TgBot::InlineKeyboardButton::Ptr clear_button(new TgBot::InlineKeyboardButton);
            clear_button->text = "Disable Clock Face Override";
            clear_button->callbackData = "command_clear_override";

            clear_row.push_back(clear_button);                  // push back the button to the row
            clock_faces_keyboard->inlineKeyboard.push_back(clear_row);

            // send the clock face keyboard to the user
            bot->getApi().sendMessage(message->chat->id, "\U0001F553 Clock Faces \U0001F553", nullptr, 0, clock_faces_keyboard, "Markdown");

            // SECOND KEYBOARD: clock controls

            TgBot::InlineKeyboardMarkup::Ptr clock_controls_keyboard(new TgBot::InlineKeyboardMarkup);
            std::vector<TgBot::InlineKeyboardButton::Ptr> clock_controls_row1;
            std::vector<TgBot::InlineKeyboardButton::Ptr> clock_controls_row2;
            std::vector<TgBot::InlineKeyboardButton::Ptr> clock_controls_row3;

            TgBot::InlineKeyboardButton::Ptr clock_on_button(new TgBot::InlineKeyboardButton);
            clock_on_button->text = "Clock On";
            clock_on_button->callbackData = "command_clock_on"; // everything that is not a clock face starts with command_ to differentiate
            clock_controls_row1.push_back(clock_on_button);     // this helps avoid confusion because we do not know what the user may name the clock faces

            TgBot::InlineKeyboardButton::Ptr clock_off_button(new TgBot::InlineKeyboardButton);
            clock_off_button->text = "Clock Off";
            clock_off_button->callbackData = "command_clock_off";
            clock_controls_row1.push_back(clock_off_button);

            TgBot::InlineKeyboardButton::Ptr force_weather_update(new TgBot::InlineKeyboardButton);
            force_weather_update->text = "Update Weather";
            force_weather_update->callbackData = "command_weather_update";
            clock_controls_row2.push_back(force_weather_update);

            TgBot::InlineKeyboardButton::Ptr force_date_update(new TgBot::InlineKeyboardButton);
            force_date_update->text = "Update Date";
            force_date_update->callbackData = "command_date_update";
            clock_controls_row2.push_back(force_date_update);

            TgBot::InlineKeyboardButton::Ptr reload_clock_faces(new TgBot::InlineKeyboardButton);
            reload_clock_faces->text = "Reload Config File";
            reload_clock_faces->callbackData = "command_reload_config";
            clock_controls_row3.push_back(reload_clock_faces);

            clock_controls_keyboard->inlineKeyboard.push_back(clock_controls_row1);
            clock_controls_keyboard->inlineKeyboard.push_back(clock_controls_row2);
            clock_controls_keyboard->inlineKeyboard.push_back(clock_controls_row3);

            bot->getApi().sendMessage(message->chat->id, "\U0001F570 Clock Controls \U0001F570", nullptr, 0, clock_controls_keyboard, "Markdown");

            // THIRD KEYBOARD: timer controls

            TgBot::InlineKeyboardMarkup::Ptr timer_controls_keyboard(new TgBot::InlineKeyboardMarkup);
            std::vector<TgBot::InlineKeyboardButton::Ptr> timer_row1;
            std::vector<TgBot::InlineKeyboardButton::Ptr> timer_row2;

            TgBot::InlineKeyboardButton::Ptr start_button(new TgBot::InlineKeyboardButton);
            start_button->text = "Start";
            start_button->callbackData = "command_timer_start";
            timer_row1.push_back(start_button);

            TgBot::InlineKeyboardButton::Ptr pause_button(new TgBot::InlineKeyboardButton);
            pause_button->text = "Pause";
            pause_button->callbackData = "command_timer_pause";
            timer_row1.push_back(pause_button);

            TgBot::InlineKeyboardButton::Ptr cancel_button(new TgBot::InlineKeyboardButton);
            cancel_button->text = "Cancel";
            cancel_button->callbackData = "command_timer_cancel";
            timer_row2.push_back(cancel_button);

            TgBot::InlineKeyboardButton::Ptr reset_button(new TgBot::InlineKeyboardButton);
            reset_button->text = "Reset";
            reset_button->callbackData = "command_timer_reset";
            timer_row2.push_back(reset_button);

            timer_controls_keyboard->inlineKeyboard.push_back(timer_row1);
            timer_controls_keyboard->inlineKeyboard.push_back(timer_row2);

            bot->getApi().sendMessage(message->chat->id, "\U0000231A Timer Controls \U0000231A", nullptr, 0, timer_controls_keyboard, "Markdown");

            // FOURTH KEYBOARD: system controls

            TgBot::InlineKeyboardMarkup::Ptr system_controls_keyboard(new TgBot::InlineKeyboardMarkup);
            std::vector<TgBot::InlineKeyboardButton::Ptr> system_row;
            std::vector<TgBot::InlineKeyboardButton::Ptr> data_row;

            TgBot::InlineKeyboardButton::Ptr ping_button(new TgBot::InlineKeyboardButton);
            ping_button->text = "Ping Clock";
            ping_button->callbackData = "command_ping";
            system_row.push_back(ping_button);

            TgBot::InlineKeyboardButton::Ptr chat_id_button(new TgBot::InlineKeyboardButton);
            chat_id_button->text = "Chat ID";
            chat_id_button->callbackData = "command_chatid";
            system_row.push_back(chat_id_button);

            TgBot::InlineKeyboardButton::Ptr print_button(new TgBot::InlineKeyboardButton);
            print_button->text = "Environment Data";
            print_button->callbackData = "command_print_data";
            data_row.push_back(print_button);

            system_controls_keyboard->inlineKeyboard.push_back(system_row);
            system_controls_keyboard->inlineKeyboard.push_back(data_row);

            bot->getApi().sendMessage(message->chat->id, "\U0001F916 System Controls \U0001F916", nullptr, 0, system_controls_keyboard, "Markdown");
        });

        bot->getEvents().onCommand("timer", [&bot, &var_util] (TgBot::Message::Ptr message) {
            if (message->chat->type == TgBot::Chat::Type::Private) {
                bot->getApi().deleteMessage(message->chat->id, message->messageId);
            }

            std::vector<std::string> split = StringTools::split(message->text, ' ');

            if (split.size() >= 3) {
                int hour = 0, minute, second;

                if (split.size() == 3) {
                    minute = std::stoi(split[1]);
                    second = std::stoi(split[2]);
                } else {
                    hour = std::stoi(split[1]);
                    minute = std::stoi(split[2]);
                    second = std::stoi(split[3]);
                }

                matrix_clock::matrix_timer* timer = new matrix_clock::matrix_timer(hour, minute, second);
                var_util->set_timer(timer);

                TgBot::InlineKeyboardMarkup::Ptr timer_controls_keyboard(new TgBot::InlineKeyboardMarkup);
                std::vector<TgBot::InlineKeyboardButton::Ptr> timer_row;

                TgBot::InlineKeyboardButton::Ptr start_button(new TgBot::InlineKeyboardButton);
                start_button->text = "Start";
                start_button->callbackData = "command_timer_start";
                timer_row.push_back(start_button);

                TgBot::InlineKeyboardButton::Ptr cancel_button(new TgBot::InlineKeyboardButton);
                cancel_button->text = "Cancel";
                cancel_button->callbackData = "command_timer_cancel";
                timer_row.push_back(cancel_button);

                TgBot::InlineKeyboardButton::Ptr dismiss_button(new TgBot::InlineKeyboardButton);   // add a dismiss button for ease of clearing push notifications
                dismiss_button->text = "Dismiss";
                dismiss_button->callbackData = "command_dismiss";
                timer_row.push_back(dismiss_button);

                timer_controls_keyboard->inlineKeyboard.push_back(timer_row);

                std::stringstream timer_info;
                timer_info << "Created a timer for ";

                if (hour != 0) timer_info << hour << " hour(s) ";
                if ((hour != 0 && minute != 0) || (hour != 0 && second != 0)) timer_info << "and ";
                if (minute != 0) timer_info << minute << " minute(s) ";
                if (minute != 0 && second != 0) timer_info << "and ";
                if (second != 0) timer_info << second << " second(s).";

                // send the user information about the timer they created as well as timer controls again
                bot->getApi().sendMessage(message->chat->id, timer_info.str(), nullptr, 0, get_timer_controls(), "Markdown");
            } else {
                bot->getApi().sendMessage(message->chat->id, "Invalid use of command. Proper usage /timer [h] [m] [s] or /timer [m] [s]");
            }
        });

        bot->getEvents().onCommand("stopwatch", [&bot, &var_util] (TgBot::Message::Ptr message) {
            if (message->chat->type == TgBot::Chat::Type::Private) {
                bot->getApi().deleteMessage(message->chat->id, message->messageId);
            }

            bot->getApi().sendMessage(message->chat->id, "Created a stopwatch.", nullptr, 0, get_timer_controls(), "Markdown");

            matrix_clock::matrix_timer* timer = new matrix_clock::matrix_timer(-2, -2, -2);
            var_util->set_timer(timer);
        });

        // callback query to the inline clock_faces_keyboard
        bot->getEvents().onCallbackQuery([&bot, &container, &var_util](TgBot::CallbackQuery::Ptr query) {
            if (!StringTools::startsWith(query->data, "command")) { // make sure it doesnt start with command, there are other buttons
                container->update_clock_face(query->data); // set the current clock face to the name pressed
                container->set_clock_face_override(true);       // set the clock face override on, this way it will stay and not change with time
                container->set_update_required(true);       // force clock update now
            } else {
                if (query->data == "command_clear_override") {
                    int times[4];
                    var_util->get_time(times);

                    container->update_clock_face(times[3], times[1], var_util->get_day_of_week());   // update the clock face to the one it should be at the current time
                    container->set_update_required(true);                   // force update
                    container->set_clock_face_override(false);              // turn off clock face override
                } else if (query->data == "command_clock_on") {
                    container->set_clock_on(true);              // turn the clock on and force update
                    container->set_update_required(true);
                } else if (query->data == "command_clock_off") {
                    container->set_clock_on(false);             // turn clock off and force update so it goes to black
                    container->set_update_required(true);
                } else if (query->data == "command_weather_update") {
                    var_util->poll_weather();                   // refresh weather and force update
                    container->set_update_required(true);
                } else if (query->data == "command_date_update") {
                    var_util->poll_date();
                    container->set_update_required(true);
                } else if (query->data == "command_ping") {
                    send_dismiss_keyboard("Bot is working correctly!", bot, query->message->chat->id);
                } else if (query->data == "command_reload_config") {
                    container->set_skip_second(1); // skip a second because of the config update
                    container->load_clock_data();
                    var_util->set_weather_url(container->get_weather_url());    // update the weather URL in case it changed and poll weather again
                    var_util->poll_weather();
                    container->set_update_required(true);       // force update
                    send_dismiss_keyboard("Successfully reloaded matrix config.", bot, query->message->chat->id);
                } else if (query->data == "command_chatid") {
                    std::stringstream stream;
                    stream << "Chat ID: " << query->message->chat->id;
                    send_dismiss_keyboard(stream.str(), bot, query->message->chat->id);
                } else if (query->data == "command_print_data") {
                    int times[4];
                    var_util->get_time(times);

                    // build the string so we only print out one message
                    std::stringstream stream;

                    // print out the date
                    stream << "Today is " << var_util->get_day_name() << ", " << var_util->get_month_name() << " ";
                    stream << var_util->get_day_of_month() << ", " << var_util->get_year() << std::endl;

                    // print out the time
                    stream << "It is currently " << times[0] << ":";
                    stream << (times[1] < 10 ? "0" : "") << times[1];
                    stream << (times[3] < 12 ? "am" : "pm") << std::endl << std::endl;

                    // no need to print out fake data if it is not correct
                    if (var_util->get_current_forecast_short() != "~Error~") {
                        // print out the weather
                        stream << "Today's weather forecast: " << var_util->get_day_forecast() << std::endl;
                        stream << "The current conditions are " << var_util->get_current_forecast();
                        stream << " (" << var_util->get_current_forecast_short() << ")." << std::endl;
                        stream << "It is currently " << var_util->get_temp() << "F (" << var_util->get_real_feel() << "F real feel) ";
                        stream << "with a humidity of " << var_util->get_humidity() << "%" << std::endl;
                        stream << "The days low is " << var_util->get_day_low() << "F with a high of " << var_util->get_day_high() << "F " << std::endl;
                        stream << "The wind is currently blowing at " << var_util->get_wind_speed() << "mph" << std::endl;
                    } else {
                        stream << "Could not load current weather data. Check your weather URL in your matrix config file." << std::endl;
                    }

                    // send the build stream to the user
                    send_dismiss_keyboard(stream.str(), bot, query->message->chat->id);
                } else if (query->data == "command_dismiss") {
                    bot->getApi().deleteMessage(query->message->chat->id, query->message->messageId);
                } else if (query->data == "command_timer_start") {
                    if (var_util->has_timer()) {
                        var_util->get_timer()->start_timer();       // only start the timer if we have one
                        container->set_skip_second(2);           // skip two seconds because the timer may be offset from telegram
                        container->set_update_required(true);       // force update
                        bot->getApi().deleteMessage(query->message->chat->id, query->message->messageId);
                    } else {
                        send_dismiss_keyboard("There is no timer to start.", bot, query->message->chat->id);
                    }
                } else if (query->data == "command_timer_pause") {
                    if (var_util->has_timer()) {
                        if (var_util->get_timer()->is_started()) {
                            var_util->get_timer()->pause();     // pause the timer to stop it from ticking (this is a toggle)
                            digitalWrite(container->get_buzzer_pin(), LOW);
                        } else {
                            send_dismiss_keyboard("This timer was never started.", bot, query->message->chat->id);
                        }
                    } else {
                        send_dismiss_keyboard("There is no timer to pause.", bot, query->message->chat->id);
                    }
                } else if (query->data == "command_timer_cancel") {
                    var_util->get_timer()->end_timer();
                    var_util->set_timer(new matrix_clock::matrix_timer(-1, 0, 0));  // set to an empty timer
                    container->set_update_required(true);   // force update to go back to the current clock face
                    digitalWrite(container->get_buzzer_pin(), LOW);   // turn off buzzer in case it was on
                } else if (query->data == "command_timer_reset") {
                    var_util->get_timer()->reset_timer();
                    container->set_update_required(true);   // force update to display the resetted timer
                    digitalWrite(container->get_buzzer_pin(), LOW);
                }
            }
        });

        TgBot::TgLongPoll long_poll(*bot); // this starts the poll

        bool warn_once = true;

        while (true) {      // loop forever (this runs again on callback; dont worry this is on a separate thread from main)
            try {
                long_poll.start();  // run the poll again after next data
            } catch (std::exception& e) {       // this is in case network drops, prevents crashes on unstable networks
                if (warn_once) {
                    std::cout << e.what() << std::endl;
                    std::cout << "Could not update telegram bot. Please check your network connection or bot token as defined in the matrix config file." << std::endl;
                    std::cout << "Service will resume once a new connection is found. If you did not want telegram bot service, please set your bot token to \"disabled\" in your matrix config file." << std::endl;
                    warn_once = false;
                }
            }
        }
    }

    void push_telegram_separate_thread(std::string message, TgBot::Bot* bot, std::int64_t chat_id, bool dismissable) {
        if (dismissable) {
            TgBot::InlineKeyboardMarkup::Ptr message_keyboard(new TgBot::InlineKeyboardMarkup);
            std::vector<TgBot::InlineKeyboardButton::Ptr> dismiss_button_row;

            TgBot::InlineKeyboardButton::Ptr dismiss_button(new TgBot::InlineKeyboardButton);   // add a dismiss button for ease of clearing push notifications
            dismiss_button->text = "Dismiss";
            dismiss_button->callbackData = "command_dismiss";

            dismiss_button_row.push_back(dismiss_button);
            message_keyboard->inlineKeyboard.push_back(dismiss_button_row);

            // set the keyboards title to be the push notification so the dismiss button is under
            bot->getApi().sendMessage(chat_id, message, nullptr, 0, message_keyboard, "Markdown");
        } else {
            bot->getApi().sendMessage(chat_id, message);
        }
    }

    void matrix_telegram::send_message(std::string message, bool dismiss_button) const {
        std::thread send_async(push_telegram_separate_thread, message, bot, chat_id, dismiss_button);   // send asynchronous to prevent visual stutters in the clock update loop
        send_async.detach();
    }

    void send_dismiss_keyboard(std::string message, TgBot::Bot* bot, std::int64_t chat_id) {
        std::thread send_async(push_telegram_separate_thread, message, bot, chat_id, true);
        send_async.detach();
    }

    TgBot::InlineKeyboardMarkup::Ptr get_timer_controls(void) {
        TgBot::InlineKeyboardMarkup::Ptr timer_controls_keyboard(new TgBot::InlineKeyboardMarkup);
        std::vector<TgBot::InlineKeyboardButton::Ptr> timer_row;

        TgBot::InlineKeyboardButton::Ptr start_button(new TgBot::InlineKeyboardButton);
        start_button->text = "Start";
        start_button->callbackData = "command_timer_start";
        timer_row.push_back(start_button);

        TgBot::InlineKeyboardButton::Ptr cancel_button(new TgBot::InlineKeyboardButton);
        cancel_button->text = "Cancel";
        cancel_button->callbackData = "command_timer_cancel";
        timer_row.push_back(cancel_button);

        TgBot::InlineKeyboardButton::Ptr dismiss_button(new TgBot::InlineKeyboardButton);   // add a dismiss button for ease of clearing push notifications
        dismiss_button->text = "Dismiss";
        dismiss_button->callbackData = "command_dismiss";
        timer_row.push_back(dismiss_button);

        timer_controls_keyboard->inlineKeyboard.push_back(timer_row);

        return timer_controls_keyboard;
    }
}