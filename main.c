#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_CARDS 10000
#define MAX_FIELD 2048

typedef struct {
  char front[MAX_FIELD];
  char back[MAX_FIELD];
} Card;

typedef struct {
  char front[MAX_FIELD];
  long next_day;
  int interval;
  int reps;
  float ef;
} Progress;

static Card cards[MAX_CARDS];
static int n_cards = 0;

static Progress progress[MAX_CARDS];
static int n_progress = 0;
