#include <ctype.h>

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0xB8, 0x64, 0xF3, 0x8A, 0xF4, 0xEA, 0x47, 0x09, 0xB3, 0xE3, 0xF1, 0x30, 0x89, 0xEA, 0x0A, 0xF5 }
PBL_APP_INFO(MY_UUID,
             "mSimplicity", "Manki",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

TextLayer text_date_layer;
TextLayer text_time_layer;
TextLayer text_other_time_layer;

Layer line_layer;


void line_layer_update_callback(Layer *me, GContext* ctx) {
  (void)me;

  graphics_context_set_stroke_color(ctx, GColorWhite);

  graphics_draw_line(ctx, GPoint(8, 97), GPoint(131, 97));
  graphics_draw_line(ctx, GPoint(8, 98), GPoint(131, 98));
}


void init_text_layer(TextLayer* text_layer, uint32_t font_resource_id) {
  text_layer_init(text_layer, window.layer.frame);
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_font(text_layer, fonts_load_custom_font(resource_get_handle(font_resource_id)));
}


PblTm add_time(const PblTm* time, unsigned int hours, unsigned int minutes) {
  PblTm result = *time;

  int result_minutes = minutes + time->tm_min;
  if (result_minutes >= 60) {
    result_minutes = result_minutes - 60;
    hours++;
  }
  result.tm_min = result_minutes;

  int result_hours = hours + time->tm_hour;
  if (result_hours >= 24) {
    result_hours = result_hours - 24;
  }
  result.tm_hour = result_hours;

  return result;
}


PblTm subtract_time(const PblTm* time, unsigned int hours, unsigned int minutes) {
  PblTm result = *time;

  int result_minutes = time->tm_min - minutes;
  if (result_minutes < 0) {
    result_minutes = 60 + result_minutes;
    hours++;
  }
  result.tm_min = result_minutes;

  int result_hours = time->tm_hour - hours;
  if (result_hours < 0) {
    result_hours = 12 - result_hours;
  }
  result.tm_hour = result_hours;

  return result;
}


// Kludge to handle lack of non-padded hour format string
// for twelve hour clock.
void maybe_remove_leading_zero(char* time_text, size_t bufsize) {
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], bufsize - 1);
  }
}


void show_time(const PblTm* time) {
  // Need to be static because they're used by the system later.
  static char date_text[] = "Xxx, Xxx 00";
  static char last_date_text[] = "Xxx, Xxx 00";
  string_format_time(date_text, sizeof(date_text), "%a, %b %e", time);
  if (strncmp(last_date_text, date_text, sizeof(date_text))) {
    text_layer_set_text(&text_date_layer, date_text);
    strncpy(last_date_text, date_text, sizeof(date_text));
  }

  const char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  static char time_text[] = "00:00";
  string_format_time(time_text, sizeof(time_text), time_format, time);
  maybe_remove_leading_zero(time_text, sizeof(time_text));
  text_layer_set_text(&text_time_layer, time_text);

  static char other_time_text[] = "XXX  XX:XX XX";
  PblTm other_time = add_time(time, 5, 30);
  string_format_time(
      other_time_text, sizeof(other_time_text), "SYD  %I:%M %p", &other_time);
  maybe_remove_leading_zero(&(other_time_text[5]), sizeof(other_time_text));
  text_layer_set_text(&text_other_time_layer, other_time_text);
}


void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "mSimplicity");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  init_text_layer(&text_date_layer, RESOURCE_ID_FONT_MONDA_21);
  layer_set_frame(&text_date_layer.layer, GRect(8, 68, 144-8, 168-68));
  layer_add_child(&window.layer, &text_date_layer.layer);

  init_text_layer(&text_time_layer, RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49);
  layer_set_frame(&text_time_layer.layer, GRect(7, 92, 144-7, 168-92));
  layer_add_child(&window.layer, &text_time_layer.layer);

  init_text_layer(&text_other_time_layer, RESOURCE_ID_FONT_MONDA_BOLD_SUBSET_16);
  layer_set_frame(&text_other_time_layer.layer, GRect(14, 10, 144-14, 25));
  layer_add_child(&window.layer, &text_other_time_layer.layer);

  layer_init(&line_layer, window.layer.frame);
  line_layer.update_proc = &line_layer_update_callback;
  layer_add_child(&window.layer, &line_layer);

  PblTm now;
  get_time(&now);
  show_time(&now);
}


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  show_time(t->tick_time);
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    }

  };
  app_event_loop(params, &handlers);
}
