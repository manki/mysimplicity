#include <ctype.h>

#include "pebble.h"


static Window *window;

static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static TextLayer *text_other_time_layer;
static TextLayer *text_battery_layer;

static Layer* window_layer;
static Layer* line_layer;

static int time_zone_known = 0;
static int utc_offset_hours = 0;
static int utc_offset_mins = 0;


void line_layer_update_callback(Layer *me, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(me), 0, GCornerNone);
}


struct tm add_time(const struct tm* time, unsigned int hours, unsigned int minutes) {
  struct tm result = *time;

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


struct tm subtract_time(const struct tm* time, unsigned int hours, unsigned int minutes) {
  struct tm result = *time;

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


void show_time(const struct tm* time) {
  // Need to be static because they're used by the system later.
  static char date_text[] = "Xxx, Xxx 00";
  static char last_date_text[] = "Xxx, Xxx 00";
  strftime(date_text, sizeof(date_text), "%a, %b %e", time);
  if (strncmp(last_date_text, date_text, sizeof(date_text))) {
    text_layer_set_text(text_date_layer, date_text);
    strncpy(last_date_text, date_text, sizeof(date_text));
  }

  const char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  static char time_text[] = "00:00";
  strftime(time_text, sizeof(time_text), time_format, time);
  maybe_remove_leading_zero(time_text, sizeof(time_text));
  text_layer_set_text(text_time_layer, time_text);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Computing time.");
  struct tm utc_time = add_time(time, utc_offset_hours, utc_offset_mins);
  // India time: UTC + 5.30
  struct tm other_time = add_time(&utc_time, 5, 30);

  static char other_time_text[] = "XX:XX XX";
  strftime(
      other_time_text, sizeof(other_time_text), "%I:%M %p", &other_time);
  maybe_remove_leading_zero(&(other_time_text[5]), sizeof(other_time_text));
  text_layer_set_text(text_other_time_layer, other_time_text);
}


void init_text_layer(TextLayer* text_layer, uint32_t font_resource_id) {
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_background_color(text_layer, GColorClear);

  GFont font = fonts_load_custom_font(
      resource_get_handle(font_resource_id));
  text_layer_set_font(text_layer, font);

  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}


void init_ui() {
  // NOTE: Pebble has a 144x168 screen.

  window = window_create();
  window_layer = window_get_root_layer(window);

  const bool animated = true;
  window_stack_push(window, animated);
  window_set_background_color(window, GColorBlack);

  text_date_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
  init_text_layer(text_date_layer, RESOURCE_ID_FONT_MONDA_21);

  text_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
  init_text_layer(text_time_layer, RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49);

  text_other_time_layer = text_layer_create(GRect(7, 3, 100, 20));
  init_text_layer(text_other_time_layer,
      RESOURCE_ID_FONT_MONDA_BOLD_SUBSET_16);

  text_battery_layer = text_layer_create(GRect(101, 3, 144-101-3, 20));
  text_layer_set_text_color(text_battery_layer, GColorWhite);
  text_layer_set_background_color(text_battery_layer, GColorClear);
  text_layer_set_text_alignment(text_battery_layer, GTextAlignmentRight);
  text_layer_set_font(text_battery_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_battery_layer));

  line_layer = layer_create(GRect(8, 97, 139, 2));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);
}


void update_battery() {
  uint8_t battery_level = battery_state_service_peek().charge_percent;
  static char battery_status_text[11] = "";
  int i;
  for (i = 0; battery_level > 20; battery_level -= 20) {
    battery_status_text[i++] = ':';
    battery_status_text[i++] = ' ';
  }
  battery_status_text[i++] = (battery_level > 10) ? ':' : '.';
  battery_status_text[i++] = '\0';

  text_layer_set_text(text_battery_layer, battery_status_text);
}


void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
  if (units_changed & MINUTE_UNIT) {
    show_time(tick_time);
  }
  if ((units_changed & HOUR_UNIT)
      || (battery_state_service_peek().is_charging)) {
    update_battery();
  }
}


void update_clock() {
  time_t t = time(NULL);
  struct tm* now = localtime(&t);
  show_time(now);
}


void handle_incoming_message(DictionaryIterator* msg, void* context) {
  static const int TIME_ZONE_OFFSET_MINUTES = 0;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got message");
  Tuple* tuple = dict_find(msg, TIME_ZONE_OFFSET_MINUTES);
  if (tuple && tuple->type == TUPLE_INT) {
    int time_zone_offset_minutes = tuple->value->int32;
    time_zone_known = 1;
    utc_offset_mins = time_zone_offset_minutes % 60;
    utc_offset_hours = time_zone_offset_minutes / 60;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Offset: %d hours %d minutes",
        utc_offset_hours, utc_offset_mins);
    update_clock();
  }
}


void init() {
  init_ui();
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

  app_message_register_inbox_received(handle_incoming_message);
  app_message_open(64, 64);

  update_clock();
  update_battery();
}


void deinit() {
  tick_timer_service_unsubscribe();
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_other_time_layer);
  window_destroy(window);
}


int main() {
  init();
  app_event_loop();
  deinit();
}
