#include "recall.h"

static Window *s_window;
static TextLayer *s_text_layer;
static TextLayer *s_hint_layer;
static ScrollLayer *s_scroll_layer;

static int s_current_card_idx = 0;
static bool s_showing_front = true;
static int s_current_deck_id = 0;

static void update_ui(void) {
  if (s_num_cards == 0) {
    text_layer_set_text(s_text_layer, "No cards due!");
    text_layer_set_text(s_hint_layer, "");
    return;
  }
  
  if (s_current_card_idx >= s_num_cards) {
    text_layer_set_text(s_text_layer, "Done!");
    text_layer_set_text(s_hint_layer, "");
    return;
  }
  
  if (s_showing_front) {
    text_layer_set_text(s_text_layer, s_cards[s_current_card_idx].front);
    text_layer_set_text(s_hint_layer, "Select: Flip");
  } else {
    text_layer_set_text(s_text_layer, s_cards[s_current_card_idx].back);
    text_layer_set_text(s_hint_layer, "Up: Easy | Sel: Good | Dn: Hard | Back: Forgot");
  }
  
  // Update content size for scroll layer
  GSize max_size = text_layer_get_content_size(s_text_layer);
  text_layer_set_size(s_text_layer, GSize(144, max_size.h + 10));
  scroll_layer_set_content_size(s_scroll_layer, GSize(144, max_size.h + 10));
}

static void send_rating(int rating) {
  if (s_current_card_idx >= s_num_cards) return;
  
  loading_window_push("Saving...");
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter) {
    dict_write_cstring(iter, AppKeyAction, "review");
    dict_write_int32(iter, AppKeyCardId, s_cards[s_current_card_idx].id);
    dict_write_int32(iter, AppKeyRating, rating);
    app_message_outbox_send();
  }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_num_cards == 0 || s_current_card_idx >= s_num_cards) return;
  
  if (s_showing_front) {
    s_showing_front = false;
    update_ui();
  } else {
    // Select = Good (3)
    send_rating(3);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_showing_front) {
    // Up = Easy (4)
    send_rating(4);
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_showing_front) {
    // Down = Hard (2)
    send_rating(2);
  }
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_showing_front) {
    // Back = Forgot / Again (1)
    send_rating(1);
  } else {
    window_stack_pop(true); // Exit card view
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_scroll_layer = scroll_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h - 20));
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  // Re-override click handlers to prioritize ours
  window_set_click_config_provider(window, click_config_provider);

  s_text_layer = text_layer_create(GRect(4, 4, bounds.size.w - 8, 2000));
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
  
  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
  
  s_hint_layer = text_layer_create(GRect(0, bounds.size.h - 20, bounds.size.w, 20));
  text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_hint_layer));

  update_ui();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_hint_layer);
  scroll_layer_destroy(s_scroll_layer);
  window_destroy(s_window);
  s_window = NULL;
}

void card_window_push(int deck_id) {
  s_current_deck_id = deck_id;
  s_current_card_idx = 0;
  s_showing_front = true;
  
  if(!s_window) {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  window_stack_push(s_window, true);
}

void card_window_update(void) {
  // Usually called after a rating was submitted successfully
  s_current_card_idx++;
  s_showing_front = true;
  
  if (s_current_card_idx >= s_num_cards) {
    // Finish deck
    window_stack_pop(true);
  } else {
    update_ui();
  }
}
