/****************************************************************************
 *   June 14 02:01:00 2021
 *   Copyright  2021  Dirk Sarodnick
 *   Email: programmer@dirk-sarodnick.de
 ****************************************************************************/
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "config.h"
#include <TTGO.h>

#include "mqtt_control_app.h"
#include "mqtt_control_app_main.h"

#include "gui/mainbar/app_tile/app_tile.h"
#include "gui/mainbar/main_tile/main_tile.h"
#include "gui/mainbar/mainbar.h"
#include "gui/statusbar.h"
#include "gui/widget_factory.h"
#include "gui/widget_styles.h"

#include "utils/mqtt/mqtt.h"
#include "utils/alloc.h"

static bool mqtt_control_state = false;

lv_obj_t *mqtt_control_main_tile = NULL;
lv_style_t mqtt_control_main_style;
lv_obj_t *mqtt_control_page = NULL;
lv_style_t mqtt_control_page_style;
lv_style_t mqtt_control_label_style;
lv_style_t mqtt_control_button_style;
lv_style_t mqtt_control_switch_style;

LV_FONT_DECLARE(Ubuntu_16px);

static void exit_mqtt_control_main_event_cb( lv_obj_t * obj, lv_event_t event );
static bool mqtt_control_mqtt_event_cb( EventBits_t event, void *arg );
static void mqtt_control_message_cb(char *topic, char *payload, size_t length);
static void mqtt_control_button_event_cb( lv_obj_t * obj, lv_event_t event );
static void mqtt_control_switch_event_cb( lv_obj_t * obj, lv_event_t event );
static void mqtt_control_item_cb( lv_obj_t * obj, lv_event_t event );

void mqtt_control_page_setup();
void mqtt_control_label_setup(size_t index, mqtt_control_item_t *mqtt_control_item);
void mqtt_control_button_setup(size_t index, mqtt_control_item_t *mqtt_control_item);
void mqtt_control_switch_setup(size_t index, mqtt_control_item_t *mqtt_control_item);
void mqtt_control_page_refresh();
void mqtt_control_page_clean();

void mqtt_control_main_setup( uint32_t tile_num ) {

    mqtt_control_main_tile = mainbar_get_tile_obj( tile_num );
    lv_style_copy( &mqtt_control_main_style, ws_get_mainbar_style() );

    lv_obj_t * exit_btn = wf_add_exit_button( mqtt_control_main_tile, exit_mqtt_control_main_event_cb, &mqtt_control_main_style );
    lv_obj_align(exit_btn, mqtt_control_main_tile, LV_ALIGN_IN_BOTTOM_LEFT, 10, -10 );

    lv_style_copy(&mqtt_control_label_style, ws_get_button_style());
    lv_style_set_text_font(&mqtt_control_label_style, LV_STATE_DEFAULT, &Ubuntu_16px);
	lv_style_set_pad_top(&mqtt_control_label_style, LV_STATE_DEFAULT, 3);
	lv_style_set_pad_left(&mqtt_control_label_style, LV_STATE_DEFAULT, 10);
	lv_style_set_pad_right(&mqtt_control_label_style, LV_STATE_DEFAULT, 10);

    lv_style_copy(&mqtt_control_button_style, ws_get_button_style());
    lv_style_set_text_font(&mqtt_control_button_style, LV_STATE_DEFAULT, &Ubuntu_16px);

    lv_style_copy(&mqtt_control_switch_style, ws_get_switch_style());
    lv_style_set_text_font(&mqtt_control_switch_style, LV_STATE_DEFAULT, &Ubuntu_16px);

    mqtt_control_page_setup();

    mqtt_register_cb( MQTT_OFF | MQTT_CONNECTED | MQTT_DISCONNECTED , mqtt_control_mqtt_event_cb, "mqtt control" );
    //TODO: Find out why it freezes within the callback
    //mqtt_register_message_cb( mqtt_control_message_cb );
}

void mqtt_control_page_setup() {
    if (mqtt_control_page) return;

    mqtt_control_page = lv_page_create(mqtt_control_main_tile, NULL);
	lv_obj_add_style(mqtt_control_page, LV_OBJ_PART_MAIN, &mqtt_control_main_style);
	lv_obj_set_pos(mqtt_control_page, 0, 0);
	lv_obj_set_size(mqtt_control_page, 240, 200);
	lv_page_set_scroll_propagation(mqtt_control_page, true);

    mqtt_control_config_t *mqtt_control_config = mqtt_control_get_config();
    for (size_t i = 0; i < MQTT_CONTROL_ITEMS; i++)
    {
        mqtt_control_item_t mqtt_control_item = mqtt_control_config->items[ i ];
        if (!strlen(mqtt_control_item.label) || !strlen(mqtt_control_item.topic)) continue;

        switch (mqtt_control_item.type) {
            case MQTT_CONTROL_TYPE_LABEL:
                mqtt_control_label_setup(i, &mqtt_control_item);
                mqtt_subscribe(mqtt_control_item.topic);
                break;
            case MQTT_CONTROL_TYPE_BUTTON:
                mqtt_control_button_setup(i, &mqtt_control_item);
                break;
            case MQTT_CONTROL_TYPE_SWITCH:
                mqtt_control_switch_setup(i, &mqtt_control_item);
                mqtt_subscribe(mqtt_control_item.topic);
                break;
        }
    }
}

void mqtt_control_label_setup(size_t index, mqtt_control_item_t *mqtt_control_item) {
    mqtt_control_item->gui_label = lv_label_create(mqtt_control_page, NULL);
	lv_obj_add_style(mqtt_control_item->gui_label, LV_OBJ_PART_MAIN, ws_get_label_style());
	lv_label_set_text(mqtt_control_item->gui_label, mqtt_control_item->label);
	lv_label_set_long_mode(mqtt_control_item->gui_label, LV_LABEL_LONG_CROP);
	lv_obj_set_pos(mqtt_control_item->gui_label, 0, 35 * index);
	lv_obj_set_size(mqtt_control_item->gui_label, 100, 25);

    mqtt_control_item->gui_object = lv_label_create(mqtt_control_page, NULL);
	lv_obj_add_style(mqtt_control_item->gui_object, LV_OBJ_PART_MAIN, &mqtt_control_label_style);
	lv_label_set_long_mode(mqtt_control_item->gui_object, LV_LABEL_LONG_SROLL_CIRC);
	lv_label_set_text(mqtt_control_item->gui_object, "");
	lv_obj_set_pos(mqtt_control_item->gui_object, 110, 35 * index);
	lv_obj_set_size(mqtt_control_item->gui_object, 100, 25);
}

void mqtt_control_button_setup(size_t index, mqtt_control_item_t *mqtt_control_item) {
    mqtt_control_item->gui_label = lv_label_create(mqtt_control_page, NULL);
	lv_obj_add_style(mqtt_control_item->gui_label, LV_OBJ_PART_MAIN, ws_get_label_style());
	lv_label_set_text(mqtt_control_item->gui_label, mqtt_control_item->label);
	lv_label_set_long_mode(mqtt_control_item->gui_label, LV_LABEL_LONG_CROP);
	lv_obj_set_pos(mqtt_control_item->gui_label, 0, 35 * index);
	lv_obj_set_size(mqtt_control_item->gui_label, 100, 25);

    mqtt_control_item->gui_object = lv_btn_create(mqtt_control_page, NULL);
	lv_obj_add_style(mqtt_control_item->gui_object, LV_OBJ_PART_MAIN, &mqtt_control_button_style);
	lv_obj_set_pos(mqtt_control_item->gui_object, 110, 35 * index);
	lv_obj_set_size(mqtt_control_item->gui_object, 100, 25);
	lv_btn_set_checkable(mqtt_control_item->gui_object, false);
	lv_btn_set_fit(mqtt_control_item->gui_object, LV_FIT_NONE);
    lv_obj_set_event_cb(mqtt_control_item->gui_object, mqtt_control_button_event_cb );
}

void mqtt_control_switch_setup(size_t index, mqtt_control_item_t *mqtt_control_item) {
    mqtt_control_item->gui_label = lv_label_create(mqtt_control_page, NULL);
	lv_obj_add_style(mqtt_control_item->gui_label, LV_OBJ_PART_MAIN, ws_get_label_style());
	lv_label_set_text(mqtt_control_item->gui_label, mqtt_control_item->label);
	lv_label_set_long_mode(mqtt_control_item->gui_label, LV_LABEL_LONG_CROP);
	lv_obj_set_pos(mqtt_control_item->gui_label, 0, 35 * index);
	lv_obj_set_size(mqtt_control_item->gui_label, 100, 25);

    mqtt_control_item->gui_object = lv_switch_create(mqtt_control_page, NULL);
	lv_obj_add_style(mqtt_control_item->gui_object, LV_OBJ_PART_MAIN, &mqtt_control_switch_style);
	lv_obj_set_pos(mqtt_control_item->gui_object, 110, 35 * index);
	lv_obj_set_size(mqtt_control_item->gui_object, 100, 25);
    lv_obj_set_event_cb(mqtt_control_item->gui_object, mqtt_control_switch_event_cb );
}

void mqtt_control_page_refresh() {
    if (!mqtt_control_page) return;

    mqtt_control_config_t *mqtt_control_config = mqtt_control_get_config();
    for (size_t i = 0; i < MQTT_CONTROL_ITEMS; i++)
    {
        mqtt_control_item_t mqtt_control_item = mqtt_control_config->items[ i ];
        if (mqtt_control_item.type == MQTT_CONTROL_TYPE_NONE) continue;
        if (!strlen(mqtt_control_item.topic)) continue;

        switch (mqtt_control_item.type) {
            case MQTT_CONTROL_TYPE_LABEL:
                mqtt_subscribe(mqtt_control_item.topic);
                break;
            case MQTT_CONTROL_TYPE_SWITCH:
                mqtt_subscribe(mqtt_control_item.topic);
                break;
        }
    }
}

void mqtt_control_page_clean() {
    if (!mqtt_control_page) return;

    mqtt_control_config_t *mqtt_control_config = mqtt_control_get_config();
    for (size_t i = 0; i < MQTT_CONTROL_ITEMS; i++)
    {
        mqtt_control_item_t mqtt_control_item = mqtt_control_config->items[ i ];
        if (mqtt_control_item.type == MQTT_CONTROL_TYPE_NONE) continue;
        if (!strlen(mqtt_control_item.topic)) continue;

        switch (mqtt_control_item.type) {
            case MQTT_CONTROL_TYPE_LABEL:
                mqtt_unsubscribe(mqtt_control_item.topic);
                break;
            case MQTT_CONTROL_TYPE_SWITCH:
                mqtt_unsubscribe(mqtt_control_item.topic);
                break;
        }
    }

    lv_obj_clean(mqtt_control_page);
    lv_obj_del(mqtt_control_page);
    mqtt_control_page = 0;
}

static bool mqtt_control_mqtt_event_cb( EventBits_t event, void *arg ) {
    switch( event ) {
        case MQTT_OFF:          mqtt_control_state = false;
                                mqtt_control_app_hide_indicator();
                                break;
        case MQTT_CONNECTED:    mqtt_control_state = true;
                                mqtt_control_app_set_indicator( ICON_INDICATOR_OK );
                                mqtt_control_page_refresh();
                                break;
        case MQTT_DISCONNECTED: mqtt_control_state = false;
                                mqtt_control_app_set_indicator( ICON_INDICATOR_FAIL );
                                break;
    }
    return( true );
}

static void mqtt_control_message_cb(char *topic, char *payload, size_t length) {
    if (!mqtt_control_state) return;
    if (!length) return;
    
    mqtt_control_config_t *mqtt_control_config = mqtt_control_get_config();
    for (size_t i = 0; i < MQTT_CONTROL_ITEMS; i++)
    {
        mqtt_control_item_t mqtt_control_item = mqtt_control_config->items[ i ];
        if (mqtt_control_item.type == MQTT_CONTROL_TYPE_NONE) continue;
        if (!strlen(mqtt_control_item.topic)) continue;

        if (mqtt_control_item.type != MQTT_CONTROL_TYPE_LABEL &&
            mqtt_control_item.type != MQTT_CONTROL_TYPE_SWITCH) continue;

        if (strncmp(topic, mqtt_control_item.topic, strlen(mqtt_control_item.topic)) != 0) continue;

        char *payload_msg = NULL;
        payload_msg = (char*)CALLOC( length + 1, 1 );
        if ( payload_msg == NULL ) {
            log_e("calloc failed");
            return;
        }
        memcpy( payload_msg, payload, length );

        switch (mqtt_control_item.type) {
            case MQTT_CONTROL_TYPE_LABEL:
                lv_label_set_text(mqtt_control_item.gui_object, payload_msg);
                break;
            case MQTT_CONTROL_TYPE_SWITCH:
                if (strncmp(payload_msg, "on", 2) == 0) {
                    lv_switch_on(mqtt_control_item.gui_object, LV_ANIM_OFF);
                }
                if (strncmp(payload_msg, "off", 3) == 0) {
                    lv_switch_off(mqtt_control_item.gui_object, LV_ANIM_OFF);
                }
                break;
        }
    }
}

static void mqtt_control_button_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mqtt_control_item_cb( obj, event );
                                        break;
    }
}

static void mqtt_control_switch_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mqtt_control_item_cb( obj, event );
                                        break;
    }
}

static void mqtt_control_item_cb( lv_obj_t * obj, lv_event_t event ) {
    if (!mqtt_control_state) return;

    mqtt_control_config_t *mqtt_control_config = mqtt_control_get_config();
    for (size_t i = 0; i < MQTT_CONTROL_ITEMS; i++)
    {
        mqtt_control_item_t mqtt_control_item = mqtt_control_config->items[ i ];
        if (mqtt_control_item.type == MQTT_CONTROL_TYPE_NONE) continue;
        if (!strlen(mqtt_control_item.topic)) continue;

        if (mqtt_control_item.type != MQTT_CONTROL_TYPE_BUTTON &&
            mqtt_control_item.type != MQTT_CONTROL_TYPE_SWITCH) continue;

        //TODO: Find the best way to compare the correct lv_obj
        if (obj != mqtt_control_item.gui_object) continue;

        switch (mqtt_control_item.type) {
            case MQTT_CONTROL_TYPE_BUTTON:
                mqtt_publish(mqtt_control_item.topic, false, "1");
                break;
            case MQTT_CONTROL_TYPE_SWITCH:
                if (lv_switch_get_state(mqtt_control_item.gui_object)) {
                    mqtt_publish(mqtt_control_item.topic, false, "1");
                } else {
                    mqtt_publish(mqtt_control_item.topic, false, "0");
                }
                break;
        }
    }
}

static void exit_mqtt_control_main_event_cb( lv_obj_t * obj, lv_event_t event ) {
    switch( event ) {
        case( LV_EVENT_CLICKED ):       mainbar_jump_to_maintile( LV_ANIM_OFF );
                                        break;
    }
}