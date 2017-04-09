// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*

   Inspired by work done here https://github.com/uwerod/ardupilot_with_Hott

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
   HoTT TextMode library
*/
#include "AP_HoTT_TextMode.h"

// zero based information
#define DISPLAY_MAX_ROWS 7
#define DISPLAY_MAX_COLS 20

#define MAX_ROWS 5
#define MAX_COLS 20

#define PARAMS_PER_PAGE 6
#define PARAMS_LINE_OFFSET 1

#define HOTT_NAME "ArduPilot"

AP_HoTT_TextMode::AP_HoTT_TextMode(HOTT_TEXTMODE_MSG &msg):
    _msg(msg),
    _editmode(false),
    _page(1),
    _row(0),
    _max_pages(1)
{
    _num_parameters = sizeof(_settings) / sizeof(_settings[0]);
    _max_pages = ceil(((float)_num_parameters / PARAMS_PER_PAGE));
}

void AP_HoTT_TextMode::handle(uint8_t address)
{
    uint8_t sensor   = (address >> 4);
    uint8_t key      = (address & 0x0f);
    uint8_t param_id = _row + ((_page - 1) * PARAMS_PER_PAGE);

    static float param_value_new = 0;

    clear_screen();
    print_word(0, HOTT_NAME, false);

    // pagination
    char sign[2];
    sign[1] = 0;
    if (_page < _max_pages) {
        sign[0] = '>';
    } else {
        sign[0] = ' ';
    }
    print_word(DISPLAY_MAX_COLS, sign, false);

    sign[0] = '<';
    print_word(DISPLAY_MAX_COLS - 1, sign, false);

    // page counter
    char page_buffer[3];
    itoa(_page, page_buffer, 10);
    print_word(15, page_buffer, false);
    print_word(16, "/", false);
    itoa(_max_pages, page_buffer, 10);
    print_word(17, page_buffer, false);

    // print page content
    uint8_t params_per_page = print_page(param_value_new);

    // handle navigation
    if (!_editmode) {
        perform_navigation(key, param_id, param_value_new, params_per_page);
    } else {
        perform_edit(key, param_id, param_value_new);
    }

    if (_page < 1) {
        _msg.fill1 = 0x01;
        _page = 1;
    } else {
        _msg.fill1 = sensor;
    }
}

void AP_HoTT_TextMode::clear_screen()
{
    // fill with spaces (ascii code 32)
    memset(_msg.msg_txt, ' ', sizeof(_msg.msg_txt));
}

void AP_HoTT_TextMode::print_word(uint8_t pos, char const *text, bool inverted)
{
    for (uint8_t index = 0; ; index++) {
        if (text[index] == 0x0) {
            break;
        } else {
            _msg.msg_txt[pos + index] = text[index] + (inverted ? 128 : 0);
        }
    }
}

void AP_HoTT_TextMode::perform_navigation(uint8_t key, uint8_t param_id, float &param_value_new, uint8_t params_per_page)
{
    switch (key) {
    case TEXT_MODE_KEY_RIGHT:
        // next page
        if (_page < _max_pages) {
            _page++;
            _row = 0;
        }
        break;

    case TEXT_MODE_KEY_LEFT:
        // previous page
        if (_page > 0) {
            _page--;
            _row = 0;
        }
        break;

    case TEXT_MODE_KEY_INC:
        // next row
        if (_row < params_per_page) {
            _row++;
        }
        break;

    case TEXT_MODE_KEY_DEC:
        // previous row
        if (_row > 0) {
            _row--;
        }
        break;
    case TEXT_MODE_KEY_SET:
        if (_settings[param_id].editable) {
            _editmode = true;

            // read current value
            param_value_new = get_param_as_float(_settings[param_id].param_name);
        }
        break;
    }
}

void AP_HoTT_TextMode::perform_edit(uint8_t key, uint8_t param_id, float &param_value_new)
{
    switch (key) {
    case TEXT_MODE_KEY_RIGHT:
    case TEXT_MODE_KEY_LEFT:
        _editmode = false;
        break;

    case TEXT_MODE_KEY_INC:
        param_value_new += _settings[param_id].step_size;
        break;

    case TEXT_MODE_KEY_DEC:
        param_value_new -= _settings[param_id].step_size;
        break;

    case TEXT_MODE_KEY_SET:
        // set new value
        // @todo

        _editmode = false;
        break;
    }
}

uint8_t AP_HoTT_TextMode::print_page(float param_value_new)
{
    for (uint8_t item_idx = 0; item_idx < PARAMS_PER_PAGE; item_idx++) {
        uint8_t row_pos = (DISPLAY_MAX_COLS + 1) * (item_idx + PARAMS_LINE_OFFSET);
        uint8_t setting_index = ((_page - 1) * PARAMS_PER_PAGE) + item_idx;

        if (setting_index < _num_parameters) {
            // label
            print_word(row_pos, _settings[setting_index].param_name, false);

            // cursor
            char sign[2];
            sign[1] = 0;

            if (item_idx == _row) {
                if (_settings[setting_index].editable) {
                    sign[0] = '>';
                } else {
                    sign[0] = '*';
                }
            } else {
                sign[0] = ':';
            }

            print_word(row_pos + 12, sign, false);

            // value
            bool inverted = _editmode && _settings[setting_index].editable && (item_idx == _row);
            char buffer[10];
            if (inverted) {
                get_value_as_char(_settings[setting_index].param_name, buffer, param_value_new);
                print_word(row_pos + 13, buffer, true);
            } else {
                get_param_as_char(_settings[setting_index].param_name, buffer);
                print_word(row_pos + 13, buffer, false);
            }

        } else {
            return item_idx - 1;
        }
    }
    return PARAMS_PER_PAGE;
}

float AP_HoTT_TextMode::get_param_as_float(const char *key)
{
    AP_Param *vp;
    enum ap_var_type var_type;
    vp = AP_Param::find(key, &var_type);

    switch (var_type) {
    case AP_PARAM_INT8:
        return (float)((AP_Int8 *)vp)->get();
    case AP_PARAM_INT16:
        return (float)((AP_Int16 *)vp)->get();
    case AP_PARAM_INT32:
        return (float)((AP_Int32 *)vp)->get();
    case AP_PARAM_FLOAT:
        return (float)((AP_Float *)vp)->get();
    default:
        return 0;
    }
}

void AP_HoTT_TextMode::get_param_as_char(const char *key, char *buffer)
{
    AP_Param *vp;
    enum ap_var_type var_type;
    vp = AP_Param::find(key, &var_type);

    switch (var_type) {
    case AP_PARAM_INT8: {
        int8_t t = ((AP_Int8 *)vp)->get();
        sprintf(buffer, "%d", t);
        break;
    }

    case AP_PARAM_INT16: {
        int16_t t = ((AP_Int16 *)vp)->get();
        sprintf(buffer, "%d", t);
        break;
    }

    case AP_PARAM_INT32: {
        int32_t t = ((AP_Int32 *)vp)->get();
        sprintf(buffer, "%d", t);
        break;
    }

    case AP_PARAM_FLOAT: {
        float t = ((AP_Float *)vp)->get();
        sprintf(buffer, "%1.3f", (double)t);
        break;
    }

    default:
        sprintf(buffer, "%d", 0);
    }
}

void AP_HoTT_TextMode::get_value_as_char(const char *key, char *buffer, float value)
{
    enum ap_var_type var_type;
    AP_Param::find(key, &var_type);

    switch (var_type) {
    case AP_PARAM_INT8: {
        int8_t t = value;
        sprintf(buffer, "%d", t);
        break;
    }

    case AP_PARAM_INT16: {
        int16_t t = value;
        sprintf(buffer, "%d", t);
        break;
    }

    case AP_PARAM_INT32: {
        int32_t t = value;
        sprintf(buffer, "%d", t);
        break;
    }

    case AP_PARAM_FLOAT: {
        float t = value;
        sprintf(buffer, "%1.3f", (double)t);
        break;
    }

    default:
        sprintf(buffer, "%d", 0);
    }
}
