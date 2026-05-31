#include "recall.h"

Deck s_decks[MAX_DECKS];
int s_num_decks = 0;

Card s_cards[MAX_CARDS];
int s_num_cards = 0;

static int s_retry_count = 0;
#define MAX_RETRIES 3
static bool s_waiting_for_response = false;

static void request_decks(void);
static void loading_timeout_handler(void *data);

static void request_decks(void) {
  if (s_waiting_for_response) return; // Already sent a request

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result == APP_MSG_OK && iter) {
    dict_write_cstring(iter, MESSAGE_KEY_AppKeyAction, "get_decks");
    result = app_message_outbox_send();
    if (result == APP_MSG_OK) {
      s_waiting_for_response = true;
      app_timer_register(8000, loading_timeout_handler, NULL);
    } else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_send failed: %d", result);
      if (s_retry_count < MAX_RETRIES) {
        s_retry_count++;
        app_timer_register(1000, (AppTimerCallback)request_decks, NULL);
      }
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_begin failed: %d", result);
    if (s_retry_count < MAX_RETRIES) {
      s_retry_count++;
      app_timer_register(1000, (AppTimerCallback)request_decks, NULL);
    }
  }
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  s_waiting_for_response = false;

  Tuple *action_tuple = dict_find(iter, MESSAGE_KEY_AppKeyAction);
  Tuple *status_tuple = dict_find(iter, MESSAGE_KEY_AppKeyStatus);

  if (status_tuple) {
    if (strcmp(status_tuple->value->cstring, "error") == 0) {
      Tuple *msg = dict_find(iter, MESSAGE_KEY_AppKeyFront);
      char err_buf[64];
      snprintf(err_buf, sizeof(err_buf), "Error: %s", msg ? msg->value->cstring : "Unknown");
      loading_window_push(err_buf);
      return;
    } else if (strcmp(status_tuple->value->cstring, "review_ok") == 0) {
      loading_window_pop();
      card_window_update();
      return;
    }
  }

  if (!action_tuple) return;

  const char *action = action_tuple->value->cstring;

  if (strcmp(action, "deck_item") == 0) {
    Tuple *idx_t = dict_find(iter, MESSAGE_KEY_AppKeyIndex);
    Tuple *id_t = dict_find(iter, MESSAGE_KEY_AppKeyDeckId);
    Tuple *name_t = dict_find(iter, MESSAGE_KEY_AppKeyName);
    Tuple *count_t = dict_find(iter, MESSAGE_KEY_AppKeyCardCount);

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
    Tuple *total_t = dict_find(iter, MESSAGE_KEY_AppKeyTotal);
    if (total_t) {
      s_num_decks = total_t->value->int32;
      if (s_num_decks > MAX_DECKS) s_num_decks = MAX_DECKS;
    }
    loading_window_pop();
    deck_window_update();
    deck_window_push();
  } else if (strcmp(action, "card_item") == 0) {
    Tuple *idx_t = dict_find(iter, MESSAGE_KEY_AppKeyIndex);
    Tuple *id_t = dict_find(iter, MESSAGE_KEY_AppKeyCardId);
    Tuple *front_t = dict_find(iter, MESSAGE_KEY_AppKeyFront);
    Tuple *back_t = dict_find(iter, MESSAGE_KEY_AppKeyBack);

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
    Tuple *total_t = dict_find(iter, MESSAGE_KEY_AppKeyTotal);
    if (total_t) {
      s_num_cards = total_t->value->int32;
      if (s_num_cards > MAX_CARDS) s_num_cards = MAX_CARDS;
    }
    loading_window_pop();
    if (s_num_cards > 0) {
      card_window_push(0);
    } else {
      deck_window_update();
      loading_window_push("No cards due");
      app_timer_register(2000, (AppTimerCallback)loading_window_pop, NULL);
    }
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage dropped! Reason: %d", reason);
  s_waiting_for_response = false;
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMessage sent successfully");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "AppMessage send failed! Reason: %d", reason);
  s_waiting_for_response = false;
  if (s_retry_count < MAX_RETRIES) {
    s_retry_count++;
    app_timer_register(1000, (AppTimerCallback)request_decks, NULL);
  } else {
    loading_window_push("Connection failed");
  }
}

static void loading_timeout_handler(void *data) {
  if (s_waiting_for_response) {
    loading_window_push("No response - retry?");
    s_waiting_for_response = false;
  }
}

static void init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(2048, 512);

  loading_window_push("Fetching Decks...");

  // Delay initial request to let app_message stabilize
  app_timer_register(500, (AppTimerCallback)request_decks, NULL);
}

static void deinit(void) {
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
