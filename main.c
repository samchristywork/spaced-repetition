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

static long today_day(void) { return (long)(time(NULL) / 86400); }

static void trim_newline(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
    s[--len] = '\0';
  }
}

static int load_cards(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "Cannot open cards file: %s\n", filename);
    return -1;
  }
  char line[MAX_FIELD * 2];
  while (fgets(line, sizeof(line), f) && n_cards < MAX_CARDS) {
    trim_newline(line);
    char *tab = strchr(line, '\t');
    if (!tab)
      continue;
    *tab = '\0';
    snprintf(cards[n_cards].front, MAX_FIELD, "%s", line);
    snprintf(cards[n_cards].back, MAX_FIELD, "%s", tab + 1);
    n_cards++;
  }
  fclose(f);
  return 0;
}

static void load_progress(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f)
    return;
  char line[MAX_FIELD * 2];
  while (fgets(line, sizeof(line), f) && n_progress < MAX_CARDS) {
    trim_newline(line);
    char *p = line;
    char *tab;

    tab = strchr(p, '\t');
    if (!tab)
      continue;
    *tab = '\0';
    snprintf(progress[n_progress].front, MAX_FIELD, "%s", p);
    p = tab + 1;

    tab = strchr(p, '\t');
    if (!tab)
      continue;
    *tab = '\0';
    progress[n_progress].next_day = atol(p);
    p = tab + 1;

    tab = strchr(p, '\t');
    if (!tab)
      continue;
    *tab = '\0';
    progress[n_progress].interval = atoi(p);
    p = tab + 1;

    tab = strchr(p, '\t');
    if (!tab)
      continue;
    *tab = '\0';
    progress[n_progress].reps = atoi(p);
    p = tab + 1;

    progress[n_progress].ef = (float)atof(p);
    n_progress++;
  }
  fclose(f);
}

static int save_progress(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "Cannot write progress file: %s\n", filename);
    return -1;
  }
  for (int i = 0; i < n_progress; i++) {
    fprintf(f, "%s\t%ld\t%d\t%d\t%.2f\n", progress[i].front,
            progress[i].next_day, progress[i].interval, progress[i].reps,
            progress[i].ef);
  }
  fclose(f);
  return 0;
}
