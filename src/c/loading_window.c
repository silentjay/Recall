#include "recall.h"

static Window *s_window;
static TextLayer *s_text_layer;
static char s_current_message[32];

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 14, bounds.size.w, 30));
  text_layer_set_text(s_text_layer, s_current_message);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void loading_window_push(const char *message) {
  strncpy(s_current_message, message, sizeof(s_current_message) - 1);
  
  if(!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
    window_stack_push(s_window, true);
  } else {
    if(s_text_layer) {
      text_layer_set_text(s_text_layer, s_current_message);
    }
  }
}

void loading_window_pop(void) {
  if(s_window) {
    window_stack_remove(s_window, true);
  }
}
