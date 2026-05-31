#include "recall.h"

static Window *s_window;
static MenuLayer *s_menu_layer;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return s_num_decks;
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  int idx = cell_index->row;
  if (idx < s_num_decks) {
    char subtitle[32];
    snprintf(subtitle, sizeof(subtitle), "%d cards", s_decks[idx].card_count);
    menu_cell_basic_draw(ctx, cell_layer, s_decks[idx].name, subtitle, NULL);
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int idx = cell_index->row;
  if (idx < s_num_decks) {
    // Show loading and request cards for this deck
    loading_window_push("Loading cards...");
    
    DictionaryIterator *iter;
    if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
      dict_write_cstring(iter, MESSAGE_KEY_AppKeyAction, "get_due");
      dict_write_int32(iter, MESSAGE_KEY_AppKeyDeckId, s_decks[idx].id);
      app_message_outbox_send();
    }
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });
  
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void deck_window_push(void) {
  if(!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  window_stack_push(s_window, true);
}

void deck_window_update(void) {
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}
