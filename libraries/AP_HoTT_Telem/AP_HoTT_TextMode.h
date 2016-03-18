// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
/*
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

#ifndef __AP_HOTT_TEXTMODE_H__
#define __AP_HOTT_TEXTMODE_H__

#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <AP_Vehicle/AP_Vehicle.h>
#include "hott_msgs.h"

#if APM_BUILD_TYPE(APM_BUILD_ArduCopter)
#define HOTT_NAME "APM:Copter"
#endif

class AP_HoTT_TextMode {
public:
    // constructor
    AP_HoTT_TextMode(HOTT_TEXTMODE_MSG &msg);

    void handle(uint8_t address);

private:
    struct hott_menu {
        const char *param_name;
        bool editable;
        float step_size;
    };

    void clear_screen();

    void print_word(uint8_t pos, char const *text, bool inverted);
    
    uint8_t print_page(float param_value_new);
    
    void perform_navigation(uint8_t key, uint8_t param_id, float &param_value_new, uint8_t params_per_page);
    
    void perform_edit(uint8_t key, uint8_t param_id, float &param_value_new);
    
    float get_param_as_float(const char *key);
    
    void get_param_as_char(const char *key, char *buffer);
    
    void get_value_as_char(const char *key, char *buffer, float value);

    HOTT_TEXTMODE_MSG &_msg;
    bool _editmode;
    uint8_t _page;
    uint8_t _row;
    uint8_t _max_pages;
    uint8_t _num_parameters;
    
    const hott_menu _settings[3] = {
        // Parameter Name,  editable, step_size
	      {"FS_BATT_MAH",     false,    50},
        {"FS_BATT_VOLTAGE", false,    0.1},
        {"BATT_CAPACITY",   false,    50},
    };

};
#endif