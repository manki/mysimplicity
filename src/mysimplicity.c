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
TextLayer text_ampm_layer;

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


void show_time(const PblTm* time) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  static char date_text[] = "Xxx, Xxx 00";
  static char last_date_text[] = "Xxx, Xxx 00";

  char *time_format;

  string_format_time(date_text, sizeof(date_text), "%a, %b %e", time);
  if (strncmp(last_date_text, date_text, sizeof(date_text))) {
    text_layer_set_text(&text_date_layer, date_text);
    strncpy(last_date_text, date_text, sizeof(date_text));
  }

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  string_format_time(time_text, sizeof(time_text), time_format, time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(&text_time_layer, time_text);

  if (!clock_is_24h_style()) {
    static char ampm_text[] = "XX";
    string_format_time(ampm_text, sizeof(ampm_text), "%p", time);
    text_layer_set_text(&text_ampm_layer, ampm_text);
  }
}


void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "mSimplicity");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  init_text_layer(&text_date_layer, RESOURCE_ID_FONT_SCADA_21);
  layer_set_frame(&text_date_layer.layer, GRect(8, 68, 144-8, 168-68));
  layer_add_child(&window.layer, &text_date_layer.layer);

  init_text_layer(&text_time_layer, RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49);
  layer_set_frame(&text_time_layer.layer, GRect(7, 92, 144-7, 168-92));
  layer_add_child(&window.layer, &text_time_layer.layer);

  init_text_layer(&text_ampm_layer, RESOURCE_ID_FONT_SCADA_SUBSET_16);
  layer_set_frame(&text_ampm_layer.layer, GRect(144-7-30, 124, 144-7, 168-89));
  layer_add_child(&window.layer, &text_ampm_layer.layer);

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
