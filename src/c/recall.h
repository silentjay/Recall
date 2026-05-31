#pragma once
#include <pebble.h>

// Use auto-generated message keys from package.json
#include "message_keys.auto.h"

#define MAX_DECKS 10
#define MAX_CARDS 50
#define MAX_TEXT_LEN 100

typedef struct {
  int id;
  char name[30];
  int card_count;
} Deck;

typedef struct {
  int id;
  char front[MAX_TEXT_LEN];
  char back[MAX_TEXT_LEN];
} Card;

// Global state arrays
extern Deck s_decks[MAX_DECKS];
extern int s_num_decks;

extern Card s_cards[MAX_CARDS];
extern int s_num_cards;

// Window forward declarations
void loading_window_push(const char *message);
void loading_window_pop(void);

void deck_window_push(void);
void deck_window_update(void);

void card_window_push(int deck_id);
void card_window_update(void);
