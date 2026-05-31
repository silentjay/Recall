#pragma once
#include <pebble.h>

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

// Global state arrays (in real life, we might want to malloc these or persist them)
extern Deck s_decks[MAX_DECKS];
extern int s_num_decks;

extern Card s_cards[MAX_CARDS];
extern int s_num_cards;

// AppMessage Keys (mapped from package.json)
#define AppKeyAction 0
#define AppKeyStatus 1
#define AppKeyDeckId 2
#define AppKeyName 3
#define AppKeyCardCount 4
#define AppKeyCardId 5
#define AppKeyFront 6
#define AppKeyBack 7
#define AppKeyRating 8
#define AppKeyIndex 9
#define AppKeyTotal 10

// Window forward declarations
void loading_window_push(const char *message);
void loading_window_pop(void);

void deck_window_push(void);
void deck_window_update(void);

void card_window_push(int deck_id);
void card_window_update(void);
