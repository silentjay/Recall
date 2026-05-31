#include "recall.h"

Deck s_decks[MAX_DECKS];
int s_num_decks = 0;

Card s_cards[MAX_CARDS];
int s_num_cards = 0;

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *action_tuple = dict_find(iter, AppKeyAction);
  Tuple *status_tuple = dict_find(iter, AppKeyStatus);
  
  if (status_tuple) {
    if (strcmp(status_tuple->value->cstring, "error") == 0) {
      Tuple *msg = dict_find(iter, AppKeyFront); // Error message is passed in front
      loading_window_push(msg ? msg->value->cstring : "Error");
      return;
    } else if (strcmp(status_tuple->value->cstring, "review_ok") == 0) {
      loading_window_pop();
      card_window_update();
      return;
    }
  }

  if (action_tuple) {
    const char *action = action_tuple->value->cstring;
    
    if (strcmp(action, "deck_item") == 0) {
      Tuple *idx_t = dict_find(iter, AppKeyIndex);
      Tuple *id_t = dict_find(iter, AppKeyDeckId);
      Tuple *name_t = dict_find(iter, AppKeyName);
      Tuple *count_t = dict_find(iter, AppKeyCardCount);
      
      if (idx_t && id_t && name_t && count_t) {
        int idx = idx_t->value->int32;
        if (idx < MAX_DECKS) {
          s_decks[idx].id = id_t->value->int32;
          strncpy(s_decks[idx].name, name_t->value->cstring, sizeof(s_decks[0].name) - 1);
          s_decks[idx].card_count = count_t->value->int32;
          if (idx + 1 > s_num_decks) s_num_decks = idx + 1;
        }
      }
    } else if (strcmp(action, "decks_done") == 0) {
      Tuple *total_t = dict_find(iter, AppKeyTotal);
      if (total_t) {
        s_num_decks = total_t->value->int32;
        if (s_num_decks > MAX_DECKS) s_num_decks = MAX_DECKS;
      }
      loading_window_pop();
      deck_window_update();
      deck_window_push();
    } else if (strcmp(action, "card_item") == 0) {
      Tuple *idx_t = dict_find(iter, AppKeyIndex);
      Tuple *id_t = dict_find(iter, AppKeyCardId);
      Tuple *front_t = dict_find(iter, AppKeyFront);
      Tuple *back_t = dict_find(iter, AppKeyBack);
      
      if (idx_t && id_t && front_t && back_t) {
        int idx = idx_t->value->int32;
        if (idx < MAX_CARDS) {
          s_cards[idx].id = id_t->value->int32;
          strncpy(s_cards[idx].front, front_t->value->cstring, MAX_TEXT_LEN - 1);
          strncpy(s_cards[idx].back, back_t->value->cstring, MAX_TEXT_LEN - 1);
          if (idx + 1 > s_num_cards) s_num_cards = idx + 1;
        }
      }
    } else if (strcmp(action, "cards_done") == 0) {
      Tuple *total_t = dict_find(iter, AppKeyTotal);
      if (total_t) {
        s_num_cards = total_t->value->int32;
        if (s_num_cards > MAX_CARDS) s_num_cards = MAX_CARDS;
      }
      loading_window_pop();
      card_window_push(0); // Assuming first deck for now or passed elsewhere
    }
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "App Message Dropped! Reason: %d", reason);
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "App Message Failed to Send! Reason: %d", reason);
}

static void init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(2048, 512);
  
  loading_window_push("Fetching Decks...");
  
  // Request decks on startup
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter) {
    dict_write_cstring(iter, AppKeyAction, "get_decks");
    app_message_outbox_send();
  }
}

static void deinit(void) {
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
