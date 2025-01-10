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

static Progress *find_progress(const char *front) {
  for (int i = 0; i < n_progress; i++) {
    if (strcmp(progress[i].front, front) == 0) {
      return &progress[i];
    }
  }
  return NULL;
}

static Progress *get_or_create_progress(const char *front) {
  Progress *p = find_progress(front);
  if (p)
    return p;
  if (n_progress >= MAX_CARDS)
    return NULL;
  p = &progress[n_progress++];
  strncpy(p->front, front, MAX_FIELD - 1);
  p->next_day = today_day();
  p->interval = 1;
  p->reps = 0;
  p->ef = 2.5f;
  return p;
}

static void sm2_update(Progress *p, int quality) {
  if (quality < 3) {
    p->reps = 0;
    p->interval = 1;
  } else {
    if (p->reps == 0) {
      p->interval = 1;
    } else if (p->reps == 1) {
      p->interval = 6;
    } else {
      p->interval = (int)roundf((float)p->interval * p->ef);
    }
    p->reps++;
    p->ef +=
        0.1f - (float)(5 - quality) * (0.08f + (float)(5 - quality) * 0.02f);
    if (p->ef < 1.3f)
      p->ef = 1.3f;
  }
  p->next_day = today_day() + p->interval;
}

static void wait_for_enter(void) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

static void format_date(long day, char *buf, size_t size) {
  time_t t = (time_t)(day * 86400);
  struct tm *tm = gmtime(&t);
  strftime(buf, size, "%Y-%m-%d", tm);
}

static void print_usage(const char *prog) {
  printf("Usage: %s <command> <cards.tsv>\n\n", prog);
  printf("Commands:\n");
  printf("  --review    Review cards that are due today\n");
  printf("  --show      Print a table of all cards and their status\n");
  printf("  --help      Show this help message\n");
}

static void cmd_show(void) {
  int w_front = (int)strlen("Front");
  int w_back = (int)strlen("Back");
  for (int i = 0; i < n_cards; i++) {
    int fl = (int)strlen(cards[i].front);
    int bl = (int)strlen(cards[i].back);
    if (fl > w_front) w_front = fl;
    if (bl > w_back) w_back = bl;
  }
  if (w_front > 40) w_front = 40;
  if (w_back > 40) w_back = 40;

  printf("%-*s  %-*s  %-10s  %8s  %4s  %4s\n",
         w_front, "Front", w_back, "Back", "Next Review", "Interval", "Reps", "EF");

  int sep_len = w_front + 2 + w_back + 2 + 10 + 2 + 8 + 2 + 4 + 2 + 4;
  for (int i = 0; i < sep_len; i++) putchar('-');
  putchar('\n');

  for (int i = 0; i < n_cards; i++) {
    Progress *p = find_progress(cards[i].front);
    char date_buf[16] = "new";
    int interval = 0, reps = 0;
    float ef = 0.0f;
    if (p) {
      format_date(p->next_day, date_buf, sizeof(date_buf));
      interval = p->interval;
      reps = p->reps;
      ef = p->ef;
    }
    printf("%-*.*s  %-*.*s  %-10s  %8d  %4d  %4.2f\n",
           w_front, w_front, cards[i].front,
           w_back, w_back, cards[i].back,
           date_buf, interval, reps, ef);
  }
}

static void cmd_review(const char *progress_file) {
  long today = today_day();
  int due_count = 0;
  for (int i = 0; i < n_cards; i++) {
    Progress *p = get_or_create_progress(cards[i].front);
    if (p && p->next_day <= today)
      due_count++;
  }

  if (due_count == 0) {
    printf("No cards due for review.\n");
    save_progress(progress_file);
    return;
  }

  printf("%d card(s) due for review.\n\n", due_count);

  int reviewed = 0;
  for (int i = 0; i < n_cards; i++) {
    Progress *p = get_or_create_progress(cards[i].front);
    if (!p || p->next_day > today)
      continue;

    reviewed++;
    printf("[%d/%d] %s\n", reviewed, due_count, cards[i].front);
    printf("Press Enter to reveal...");
    fflush(stdout);
    wait_for_enter();

    printf("Answer: %s\n\n", cards[i].back);

    int quality = -1;
    char buf[64];
    while (quality < 0 || quality > 5) {
      printf("Score (0=blackout 1=wrong 2=close 3=hard 4=good 5=easy): ");
      fflush(stdout);
      if (!fgets(buf, sizeof(buf), stdin))
        break;
      if (sscanf(buf, "%d", &quality) != 1 || quality < 0 || quality > 5) {
        printf("Enter a number from 0 to 5.\n");
        quality = -1;
      }
    }

    sm2_update(p, quality);
    printf("Next review in %d day(s).\n\n", p->interval);
    save_progress(progress_file);
  }

  printf("Session complete. Reviewed %d card(s).\n", reviewed);
}

int main(int argc, char *argv[]) {
  if (argc < 2 || strcmp(argv[1], "--help") == 0) {
    print_usage(argv[0]);
    return 0;
  }

  const char *cmd = argv[1];
  if (strcmp(cmd, "--show") != 0 && strcmp(cmd, "--review") != 0) {
    fprintf(stderr, "Unknown command: %s\n\n", cmd);
    print_usage(argv[0]);
    return 1;
  }

  if (argc < 3) {
    fprintf(stderr, "Missing cards file.\n\n");
    print_usage(argv[0]);
    return 1;
  }

  const char *cards_file = argv[2];
  if (load_cards(cards_file) < 0)
    return 1;
  if (n_cards == 0) {
    fprintf(stderr, "No cards found in %s\n", cards_file);
    return 1;
  }

  char progress_file[4096];
  snprintf(progress_file, sizeof(progress_file), "%s.progress", cards_file);
  load_progress(progress_file);

  if (strcmp(cmd, "--show") == 0) {
    cmd_show();
  } else {
    cmd_review(progress_file);
  }

  return 0;
}
