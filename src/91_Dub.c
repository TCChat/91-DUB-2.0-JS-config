#include "pebble.h"
#include <ctype.h>

static int blink = 0;
static int zeroes = 0;
static int vibrate = 0;
static int square = 0;
static int date = 0;

enum {
	KEY_BLINK = 0x1,
	KEY_ZEROES = 0x2,
	KEY_VIBRATE = 0x3,
	KEY_SQUARE = 0x4,
	KEY_DATE = 0x5
};

static Window *window;

static GBitmap *background_image;
static BitmapLayer *background_layer;

static GBitmap *meter_bar_image;
static BitmapLayer *meter_bar_layer;
static BitmapLayer *meter_bar_mask_layer;

static GBitmap *bt_image;
static BitmapLayer *bt_layer;
/*
static GBitmap *batt_image;
static BitmapLayer *batt_layer;
static BitmapLayer *batt_mask_layer;
*/

static TextLayer *day_layer;
static InverterLayer *day_inv_layer;
static GFont *digital_font;

static GBitmap *dots_image;
static BitmapLayer *dots_layer;

static TextLayer *time_format_layer;

#define TOTAL_TIME_DIGITS 4

static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};
/*
#define TOTAL_DATE_DIGITS 2
	
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};
*/
static TextLayer *date_layer[2];
/*
static GBitmap *day_name_image = NULL;
static BitmapLayer *day_name_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};
*/
static bool is_hidden = false;

const TimeUnits sec_unit = SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT;
const TimeUnits no_sec_unit = MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT;

static char text_buffer[] = "24H";
static char text_buffer0[] = "JAN";
static char text_buffer1[] = "DEC";

char *upcase(char *str) {
    char *s = str;

	while (*s){
        *s++ = toupper((int)*s);
    }

    return str;
}

unsigned short get_display_hour(unsigned short hour){
  if(clock_is_24h_style()){
    return hour;
  }
  unsigned short display_hour = hour % 12;
	
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id){
  GBitmap *old_image = *bmp_image;
	
  *bmp_image = gbitmap_create_with_resource(resource_id);
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	
  gbitmap_destroy(old_image);
}

static void update_display(struct tm *current_time, TimeUnits units_changed){
  if(blink == 0){
    if(current_time->tm_sec % 2 == 0){
      layer_set_hidden(bitmap_layer_get_layer(dots_layer), false);
    } else {
      layer_set_hidden(bitmap_layer_get_layer(dots_layer), true);
    }
  }
	
  //bool is_charging = battery_state_service_peek().is_charging;
	
  if(battery_state_service_peek().is_charging){
    if(current_time->tm_sec % 2 == 0){
      //bitmap_layer_set_bitmap(meter_bar_layer, meter_bar_image);
      layer_set_hidden(bitmap_layer_get_layer(meter_bar_layer), false);
      is_hidden = false;
    } else {
      //bitmap_layer_set_bitmap(meter_bar_layer, NULL);
      layer_set_hidden(bitmap_layer_get_layer(meter_bar_layer), true);
      is_hidden = true;
      }
  } else {
    if(is_hidden){
      //bitmap_layer_set_bitmap(meter_bar_layer, meter_bar_image);
      layer_set_hidden(bitmap_layer_get_layer(meter_bar_layer), false);
      is_hidden = false;
    }
  }

  if (units_changed & MINUTE_UNIT) {
    // TODO implement minute display changes
    set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min/10]);
    set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min%10]);

    if (units_changed & HOUR_UNIT) {
      // TODO implement hourly display changes
      unsigned short display_hour = get_display_hour(current_time->tm_hour);
	
      set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10]);
      set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10]);
			
      if(display_hour < 10 && zeroes == 0) {
        layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
      } else {
	    layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
      }
		
	  if(!clock_is_24h_style()){
		  strftime(text_buffer, sizeof(text_buffer), "%p", current_time);
		  text_layer_set_text(time_format_layer, text_buffer);
	  }	else {
		  text_layer_set_text(time_format_layer, "24H");
	  }
		
		if (units_changed & DAY_UNIT) {
        // TODO implement daily display changes
			
        	//set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[current_time->tm_wday]);
			//char* formating[] = {"%b", "%e", "%b", "%d"};
			char *formating0 = "%b";
			char *formating1 = "%e";
			if(date == 0){
				formating0 ="%b";
				if(zeroes == 0){
					formating1 = "%e";
				} else {
					formating1 = "%d";
				}
			} else {
				if(zeroes == 0){
					formating0 = "%e";
				} else {
					formating0 = "%d";
				}
				formating1 = "%b";
			}
			
			//strftime(text_buffer, sizeof(text_buffer), formating[zeroes], current_time);
			strftime(text_buffer0, sizeof(text_buffer0), formating0, current_time);
			upcase(text_buffer0);
			text_layer_set_text(date_layer[0], text_buffer0);
			strftime(text_buffer1, sizeof(text_buffer1), formating1, current_time);
			upcase(text_buffer1);
			text_layer_set_text(date_layer[1], text_buffer1);
			/*
			set_container_image(&date_digits_images[0], date_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday/10]);
			set_container_image(&date_digits_images[1], date_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday%10]);
	
			if(current_time->tm_mday < 10 && zeroes == 1) {
          		layer_set_hidden(bitmap_layer_get_layer(date_digits_layers[0]), true);
			} else {
	  			layer_set_hidden(bitmap_layer_get_layer(date_digits_layers[0]), false);
	        }
			*/
			
			GRect frame = (GRect){{0, 15}, {15, 3}};
			if(square == 1){
				frame = (GRect){{0, 3}, {15, 13}};
			}
			
			//GRect original = layer_get_frame(inverter_layer_get_layer(day_inv_layer));
			int offset[] = {0, 16, 33, 48, 67, 82, 94};
			int width[] = {15, 17, 14, 19, 14, 12, 15};
				
			frame.origin.x = offset[current_time->tm_wday];
			frame.size.w = width[current_time->tm_wday];
			if(square == 0){
				frame.origin.x = frame.origin.x + 1;
				frame.size.w = frame.size.w - 2;
			}
			layer_set_frame(inverter_layer_get_layer(day_inv_layer), frame);
			
			if(units_changed & MONTH_UNIT){
				//strftime(text_buffer, sizeof(text_buffer), "%h", current_time);
				//text_layer_set_text(month_layer, text_buffer);
			}
		}
    }
  }  
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed){
  update_display(tick_time, units_changed);
}

static void handle_bluetooth(bool connected){
  if(connected){
    //bitmap_layer_set_bitmap(bt_layer, bt_image);
    layer_set_hidden(bitmap_layer_get_layer(bt_layer), false);
  } else {
    //bitmap_layer_set_bitmap(bt_layer, NULL);
    layer_set_hidden(bitmap_layer_get_layer(bt_layer), true);
	  if(vibrate == 0){
		  vibes_long_pulse();
	  }
  }
}

static void handle_battery(BatteryChargeState charge_state){
  int percentage_offset = 12;
	
  if(charge_state.charge_percent > 10){
    percentage_offset = 11;
  } 
  if(charge_state.charge_percent > 20){
    percentage_offset = 9;
  } 
  if(charge_state.charge_percent > 40){
    percentage_offset = 6;
  }	
  if(charge_state.charge_percent > 60){
    percentage_offset = 3;
  } 
  if(charge_state.charge_percent > 80){
    percentage_offset = 0;
  }
  //GRect frame = (GRect){ .origin = { .x = 17, .y = 43}, {percentage_offset, 9}};
  GRect frame = (GRect){ .origin = { .x = 0, .y = 0}, {percentage_offset, 9}};
  layer_set_frame(bitmap_layer_get_layer(meter_bar_mask_layer), frame);
	
  //TimeUnits units_changed = SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT;
  TimeUnits units_changed = sec_unit;
  if (!charge_state.is_charging && blink == 1){
	layer_set_hidden(bitmap_layer_get_layer(meter_bar_layer), false);
    is_hidden = false;
	
	units_changed = no_sec_unit;
  }
  tick_timer_service_subscribe(units_changed, handle_tick);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *tuple = dict_find(iter, KEY_BLINK);
	blink = tuple->value->uint8;
	
	TimeUnits units_changed = sec_unit;
	if (blink == 1){
		layer_set_hidden(bitmap_layer_get_layer(dots_layer), false);
		if(!battery_state_service_peek().is_charging){
			units_changed = no_sec_unit;
			
		}
	}
	tick_timer_service_subscribe(units_changed, handle_tick);
	
	tuple = dict_find(iter, KEY_ZEROES);
	zeroes = tuple->value->uint8;
	
	tuple = dict_find(iter, KEY_VIBRATE);
	vibrate = tuple->value->uint8;
	
	tuple = dict_find(iter, KEY_SQUARE);
	square = tuple->value->uint8;
	
	tuple = dict_find(iter, KEY_DATE);
	date = tuple->value->uint8;
	
 	time_t now = time(NULL);
	struct tm *tick_time = localtime(&now);
	update_display(tick_time, units_changed);
}

static void in_dropped_handler(AppMessageResult reason, void *context) {}

void init(){
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(64, 0);
	
  window = window_create();
  window_stack_push(window, true /* Animated */);	
  Layer *window_layer = window_get_root_layer(window);

	if(persist_exists(KEY_BLINK)){
		blink = persist_read_int(KEY_BLINK);
	}
	if(persist_exists(KEY_ZEROES)){
    	zeroes = persist_read_int(KEY_ZEROES);
	}
	if(persist_exists(KEY_VIBRATE)){
    	vibrate = persist_read_int(KEY_VIBRATE);
	}
	if(persist_exists(KEY_SQUARE)){
		square = persist_read_int(KEY_SQUARE);
	}
	if(persist_exists(KEY_DATE)){
		date = persist_read_int(KEY_DATE);
	}
	
  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  background_layer = bitmap_layer_create(layer_get_frame(window_layer));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));

  meter_bar_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_METER_BAR);
  GRect frame = (GRect){ .origin = { .x = 38, .y = 67}, .size = meter_bar_image->bounds.size};
  meter_bar_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(meter_bar_layer, meter_bar_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(meter_bar_layer));

  meter_bar_mask_layer = bitmap_layer_create(frame);
  bitmap_layer_set_background_color(meter_bar_mask_layer, GColorWhite);
  layer_add_child(bitmap_layer_get_layer(meter_bar_layer), bitmap_layer_get_layer(meter_bar_mask_layer));

  bt_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT);
  frame = (GRect){ .origin = { .x = 54, .y = 67}, .size = bt_image->bounds.size};
  bt_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(bt_layer, bt_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bt_layer));

	//frame = layer_get_bounds(window_layer);
	//frame = (GRect){{45, 34}, {87, 45}};
	frame = (GRect){{17, 38}, {109, 19}};
	day_layer = text_layer_create(frame);
	text_layer_set_background_color(day_layer, GColorClear);
	//text_layer_set_background_color(day_layer, GColorWhite);
  	text_layer_set_text_color(day_layer, GColorBlack);
	text_layer_set_text_alignment(day_layer, GTextAlignmentCenter);
  	text_layer_set_font(day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  	text_layer_set_text(day_layer, "Su Mo Tu We Th Fr Sa");
  	layer_add_child(window_layer, text_layer_get_layer(day_layer));
	
  	//frame = (GRect){{0, 3}, {15, 13}};
	if(square == 0){
		frame = (GRect){{0, 15}, {15, 3}};
	} else {
		frame = (GRect){{0, 3}, {15, 13}};
	}
  	day_inv_layer = inverter_layer_create(frame);
  	layer_add_child(text_layer_get_layer(day_layer), inverter_layer_get_layer(day_inv_layer));

  /*
  time_format_image = gbitmap_create_with_resource(TIME_FORMAT_IDS[0]);
  frame = (GRect){ .origin = { .x = 17, .y = 68}, .size = time_format_image->bounds.size};
  time_format_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(time_format_layer, time_format_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));
  */
	
	frame = (GRect){ .origin = { .x = 15, .y = 62}, .size = {20, 14}};
	time_format_layer = text_layer_create(frame);
	text_layer_set_background_color(time_format_layer, GColorClear);
	text_layer_set_text_color(time_format_layer, GColorBlack);
	text_layer_set_font(time_format_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text(time_format_layer, "24H");
	layer_add_child(window_layer, text_layer_get_layer(time_format_layer));
	
  dots_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOTS);
  frame = (GRect){ .origin = { .x = 69, .y = 91}, .size = dots_image->bounds.size};
  dots_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(dots_layer, dots_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(dots_layer));	
/*
  day_name_image = gbitmap_create_with_resource(DAY_NAME_IMAGE_RESOURCE_IDS[0]);
  frame = (GRect){ .origin = { .x = 69, .y = 61}, .size = day_name_image->bounds.size};
  day_name_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(day_name_layer, day_name_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));
*/	
	digital_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_22));
	//digital_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_32));
	frame = (GRect){ .origin = { .x = 66, .y = 54}, .size = {65, 40}};
	//frame = (GRect){ .origin = { .x = 75, .y = 54}, .size = {60, 22}};
	for(int i = 0; i < 2; i++){
		date_layer[i] = text_layer_create(frame);
		text_layer_set_background_color(date_layer[i], GColorClear);
		text_layer_set_text_color(date_layer[i], GColorBlack);
		text_layer_set_font(date_layer[i], digital_font);
		text_layer_set_text(date_layer[i], "JAN");
		layer_add_child(window_layer, text_layer_get_layer(date_layer[i]));
	}
	text_layer_set_text_alignment(date_layer[0], GTextAlignmentLeft);
	text_layer_set_text_alignment(date_layer[1], GTextAlignmentRight);
/*
  date_digits_images[0] = gbitmap_create_with_resource(DATENUM_IMAGE_RESOURCE_IDS[0]);
  frame = (GRect){ .origin = { .x = 108, .y = 61}, .size = date_digits_images[0]->bounds.size};
  date_digits_layers[0] = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(date_digits_layers[0], date_digits_images[0]);
  layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[0]));

  date_digits_images[1] = gbitmap_create_with_resource(DATENUM_IMAGE_RESOURCE_IDS[0]);
  frame = (GRect){ .origin = { .x = 121, .y = 61}, .size = date_digits_images[0]->bounds.size};
  date_digits_layers[1] = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(date_digits_layers[1], date_digits_images[1]);
  layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[1]));
*/
  for(int i = 0; i < TOTAL_TIME_DIGITS; i++){
    time_digits_images[i] = gbitmap_create_with_resource(BIG_DIGIT_IMAGE_RESOURCE_IDS[0]);
    GPoint point;
    switch (i){
      case 0:
        point = GPoint(10, 84);
        break;
      case 1:
        point = GPoint(40, 84);
        break;
      case 2:
        point = GPoint(77, 84);
        break;
      case 3:
        point = GPoint(105, 84);
        break;
    }
    frame = (GRect) { .origin = point, .size = time_digits_images[i]->bounds.size };
    time_digits_layers[i] = bitmap_layer_create(frame);
    bitmap_layer_set_bitmap(time_digits_layers[i], time_digits_images[i]);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }  
	
  TimeUnits units_changed = sec_unit;
  if (blink == 1){
	units_changed = no_sec_unit;
  }
	
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  update_display(tick_time, units_changed);

  tick_timer_service_subscribe(units_changed, handle_tick);
  bluetooth_connection_service_subscribe(handle_bluetooth);
  handle_bluetooth(bluetooth_connection_service_peek());
  battery_state_service_subscribe(handle_battery);
  handle_battery(battery_state_service_peek());
}

void deinit(){
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  tick_timer_service_unsubscribe();

 	int temp;
 	temp = persist_read_int(KEY_BLINK);
 	if(temp != blink){
		persist_write_int(KEY_BLINK, blink);
	}
	temp = persist_read_int(KEY_ZEROES);
	if(temp != zeroes){
		persist_write_int(KEY_ZEROES, zeroes);
	}
	temp = persist_read_int(KEY_VIBRATE);
	if(temp != vibrate){
		persist_write_int(KEY_VIBRATE, vibrate);
	}
	temp = persist_read_int(KEY_SQUARE);
	if(temp != square){
		persist_write_int(KEY_SQUARE, square);
	}
	temp = persist_read_int(KEY_DATE);
	if(temp != date){
		persist_write_int(KEY_DATE, date);
	}
	
  for(int i = 0; i < TOTAL_TIME_DIGITS; i++){
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    bitmap_layer_destroy(time_digits_layers[i]);
    gbitmap_destroy(time_digits_images[i]);
  }
/*
  for(int i = 0; i < 2; i++){
    layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    bitmap_layer_destroy(date_digits_layers[i]);
    gbitmap_destroy(date_digits_images[i]); 
  }
*/ 
	for(int i = 0; i < 2; i++){
		layer_remove_from_parent(text_layer_get_layer(date_layer[i]));
		text_layer_destroy(date_layer[i]);
	}
	fonts_unload_custom_font(digital_font);
/*	
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
*/
  layer_remove_from_parent(bitmap_layer_get_layer(dots_layer));
  bitmap_layer_destroy(dots_layer);
  gbitmap_destroy(dots_image);
/*
  layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
  bitmap_layer_destroy(time_format_layer);
  gbitmap_destroy(time_format_image);
*/
	layer_remove_from_parent(text_layer_get_layer(time_format_layer));
	text_layer_destroy(time_format_layer);
	
  inverter_layer_destroy(day_inv_layer);
  text_layer_destroy(day_layer);
	
  layer_remove_from_parent(bitmap_layer_get_layer(bt_layer));
  bitmap_layer_destroy(bt_layer);
  gbitmap_destroy(bt_image);

  layer_remove_from_parent(bitmap_layer_get_layer(meter_bar_mask_layer));
  bitmap_layer_destroy(meter_bar_mask_layer);
  layer_remove_from_parent(bitmap_layer_get_layer(meter_bar_layer));
  bitmap_layer_destroy(meter_bar_layer);
  gbitmap_destroy(meter_bar_image); 

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  window_destroy(window);
	
  app_message_deregister_callbacks();
}

int main(void){
  init();
  app_event_loop();
  deinit();
}
