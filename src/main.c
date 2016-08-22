#include <pebble.h>
#include "main.h"

static Window *s_main_window;
static GBitmap *s_owl_bitmap;
static BitmapLayer *s_owl_layer;
static TextLayer *s_time_layer, *s_date_layer;
static int s_battery_level;
static GFont s_time_font;
static Layer *s_battery_layer;

static void bluetooth_callback(bool connected) {

  if (!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
    
    s_owl_bitmap = gbitmap_create_with_resource(RESOURCE_ID_OWL_NO_BT);
    bitmap_layer_set_bitmap(s_owl_layer, s_owl_bitmap);
  } else {
    s_owl_bitmap = gbitmap_create_with_resource(RESOURCE_ID_OWL_IMG);
    bitmap_layer_set_bitmap(s_owl_layer, s_owl_bitmap);
  }
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  
  // Update meter
  layer_mark_dirty(s_battery_layer);
  
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int height = (int)(float)(((float)s_battery_level / 6.0F));

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, (24-height)/2), 0, GCornerNone);
   
  s_owl_bitmap = gbitmap_create_with_resource(RESOURCE_ID_OWL_IMG);
  
  bitmap_layer_set_bitmap(s_owl_layer, s_owl_bitmap);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  // Copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%d-%m-%Y", tick_time);
  
  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);  
}
  
  static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_owl_bitmap = gbitmap_create_with_resource(RESOURCE_ID_OWL_IMG);
  s_owl_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_owl_layer, s_owl_bitmap);
  bitmap_layer_set_alignment(s_owl_layer, GAlignLeft);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(s_owl_layer));
  
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(2, 0), bounds.size.w, 40));
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CALIBRI_BOLD_20));

  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

 
  // Add it as a child layer to the Window's root layer

  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(12, 30, bounds.size.w-20, 40));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_font(s_date_layer, s_time_font);
  // Add to Window
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(63, PBL_IF_ROUND_ELSE(75, 70), 65, 24));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  
  // Add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
//   s_owl_bitmap = gbitmap_create_with_resource(RESOURCE_ID_OWL_IMG);
//   s_owl_layer = bitmap_layer_create(bounds);
//   bitmap_layer_set_bitmap(s_owl_layer, s_owl_bitmap);
//   bitmap_layer_set_alignment(s_owl_layer, GAlignLeft);
//   layer_add_child(window_layer, bitmap_layer_get_layer(s_owl_layer));
}

static void main_window_unload(Window *window) {
  gbitmap_destroy(s_owl_bitmap);
  bitmap_layer_destroy(s_owl_layer);
  fonts_unload_custom_font(s_time_font);
  text_layer_destroy(s_date_layer);
  layer_destroy(s_battery_layer);
}

static void init() {
  s_main_window = window_create();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);
  // Make sure the time is displayed from the start
  update_time();
  
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());
  
  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });


}

static void deinit() {
  window_destroy(s_main_window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}