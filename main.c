#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_CYAN    "\033[36m"

#define MAX_CARDS 10000
#define MAX_FIELD 2048
#define MAX_FILES 64

typedef struct {
  char front[MAX_FIELD];
  char back[MAX_FIELD];
  int file_idx;
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

static char g_cards_files[MAX_FILES][4096];
static char g_progress_files[MAX_FILES][4096];
static int n_files = 0;

static long today_day(void) { return (long)(time(NULL) / 86400); }

static void shuffle_cards(void) {
  for (int i = n_cards - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    Card tmp = cards[i];
    cards[i] = cards[j];
    cards[j] = tmp;
  }
}

static void trim_newline(char *s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
    s[--len] = '\0';
  }
}

static int load_cards(const char *filename, int file_idx) {
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
    cards[n_cards].file_idx = file_idx;
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

static int save_all_progress(void) {
  int result = 0;
  for (int fi = 0; fi < n_files; fi++) {
    FILE *f = fopen(g_progress_files[fi], "w");
    if (!f) {
      fprintf(stderr, "Cannot write progress file: %s\n", g_progress_files[fi]);
      result = -1;
      continue;
    }
    for (int pi = 0; pi < n_progress; pi++) {
      for (int ci = 0; ci < n_cards; ci++) {
        if (strcmp(cards[ci].front, progress[pi].front) == 0) {
          if (cards[ci].file_idx == fi) {
            fprintf(f, "%s\t%ld\t%d\t%d\t%.2f\n", progress[pi].front,
                    progress[pi].next_day, progress[pi].interval,
                    progress[pi].reps, progress[pi].ef);
          }
          break;
        }
      }
    }
    fclose(f);
  }
  return result;
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
  printf(C_BOLD "Usage:" C_RESET " %s <command> <cards.tsv> [more.tsv ...]\n\n", prog);
  printf(C_BOLD "Commands:\n" C_RESET);
  printf("  " C_CYAN "--add" C_RESET "            Add a new card interactively\n");
  printf("  " C_CYAN "--review" C_RESET "         Review cards that are due today\n");
  printf("  " C_CYAN "--show" C_RESET "           Print a table of all cards and their status\n");
  printf("  " C_CYAN "--stats" C_RESET "          Show deck statistics\n");
  printf("  " C_CYAN "--search" C_RESET " <term>  Find cards matching a substring\n");
  printf("  " C_CYAN "--help" C_RESET "           Show this help message\n");
  printf("\n" C_BOLD "Flags:\n" C_RESET);
  printf("  " C_CYAN "--reverse" C_RESET "        Swap front and back (uses separate progress)\n");
}

static void cmd_stats(void) {
  long today = today_day();
  int n_new = 0, n_due = 0, n_due_week = 0, n_learned = 0;
  float ef_sum = 0.0f;

  for (int i = 0; i < n_cards; i++) {
    Progress *p = find_progress(cards[i].front);
    if (!p) {
      n_new++;
      continue;
    }
    n_learned++;
    ef_sum += p->ef;
    if (p->next_day <= today)
      n_due++;
    else if (p->next_day <= today + 7)
      n_due_week++;
  }

  printf(C_BOLD "Deck statistics\n" C_RESET);
  printf("  Total cards:      %d\n", n_cards);
  printf("  New (unseen):     " C_BLUE "%d\n" C_RESET, n_new);
  printf("  Due today:        %s%d\n" C_RESET, n_due > 0 ? C_YELLOW C_BOLD : C_GREEN, n_due);
  printf("  Due this week:    %d\n", n_due + n_due_week);
  if (n_learned > 0)
    printf("  Avg. ease factor: %.2f\n", (double)(ef_sum / (float)n_learned));
}

static int cmd_add(const char *cards_file) {
  char front[MAX_FIELD];
  char back[MAX_FIELD];

  printf(C_BOLD "Front: " C_RESET);
  fflush(stdout);
  if (!fgets(front, sizeof(front), stdin)) return -1;
  trim_newline(front);
  if (front[0] == '\0') {
    fprintf(stderr, "Front cannot be empty.\n");
    return -1;
  }

  printf(C_BOLD "Back:  " C_RESET);
  fflush(stdout);
  if (!fgets(back, sizeof(back), stdin)) return -1;
  trim_newline(back);
  if (back[0] == '\0') {
    fprintf(stderr, "Back cannot be empty.\n");
    return -1;
  }

  if (strchr(front, '\t') || strchr(back, '\t')) {
    fprintf(stderr, "Card text must not contain tab characters.\n");
    return -1;
  }

  FILE *f = fopen(cards_file, "a");
  if (!f) {
    fprintf(stderr, "Cannot open cards file: %s\n", cards_file);
    return -1;
  }
  fprintf(f, "%s\t%s\n", front, back);
  fclose(f);

  printf(C_GREEN "Card added." C_RESET "\n");
  return 0;
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

  printf(C_BOLD "%-*s  %-*s  %-10s  %8s  %4s  %4s\n" C_RESET,
         w_front, "Front", w_back, "Back", "Next Review", "Interval", "Reps", "EF");

  int sep_len = w_front + 2 + w_back + 2 + 10 + 2 + 8 + 2 + 4 + 2 + 4;
  printf(C_DIM);
  for (int i = 0; i < sep_len; i++) putchar('-');
  printf(C_RESET);
  putchar('\n');

  long today = today_day();
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
    const char *row_color = "";
    const char *date_color = "";
    if (!p) {
      date_color = C_BLUE;
    } else if (p->next_day <= today) {
      row_color = C_YELLOW;
      date_color = C_YELLOW C_BOLD;
    } else {
      date_color = C_GREEN;
    }
    printf("%s%-*.*s  %-*.*s  %s%-10s" C_RESET "%s  %8d  %4d  %4.2f\n" C_RESET,
           row_color,
           w_front, w_front, cards[i].front,
           w_back, w_back, cards[i].back,
           date_color, date_buf,
           row_color, interval, reps, ef);
  }
}

static void cmd_review(void) {
  long today = today_day();
  int due_count = 0;
  for (int i = 0; i < n_cards; i++) {
    Progress *p = get_or_create_progress(cards[i].front);
    if (p && p->next_day <= today)
      due_count++;
  }

  if (due_count == 0) {
    printf(C_GREEN "No cards due for review.\n" C_RESET);
    save_all_progress();
    return;
  }

  printf(C_BOLD "%d card(s) due for review.\n\n" C_RESET, due_count);

  int reviewed = 0;
  for (int i = 0; i < n_cards; i++) {
    Progress *p = get_or_create_progress(cards[i].front);
    if (!p || p->next_day > today)
      continue;

    reviewed++;
    printf(C_BOLD C_CYAN "[%d/%d]" C_RESET C_BOLD " %s\n" C_RESET,
           reviewed, due_count, cards[i].front);
    printf(C_DIM "Press Enter to reveal..." C_RESET);
    fflush(stdout);
    wait_for_enter();

    printf(C_BOLD "Answer: " C_RESET C_GREEN "%s\n\n" C_RESET, cards[i].back);

    int quality = -1;
    char buf[64];
    while (quality < 0 || quality > 5) {
      printf(C_YELLOW "Score (0=blackout 1=wrong 2=close 3=hard 4=good 5=easy): " C_RESET);
      fflush(stdout);
      if (!fgets(buf, sizeof(buf), stdin))
        break;
      if (sscanf(buf, "%d", &quality) != 1 || quality < 0 || quality > 5) {
        printf(C_RED "Enter a number from 0 to 5.\n" C_RESET);
        quality = -1;
      }
    }

    sm2_update(p, quality);
    printf(C_DIM "Next review in %d day(s).\n\n" C_RESET, p->interval);
    save_all_progress();
  }

  printf(C_BOLD C_GREEN "Session complete." C_RESET " Reviewed %d card(s).\n", reviewed);
}

static int contains_icase(const char *haystack, const char *needle) {
  size_t hlen = strlen(haystack), nlen = strlen(needle);
  if (nlen == 0) return 1;
  for (size_t i = 0; i + nlen <= hlen; i++) {
    size_t j;
    for (j = 0; j < nlen; j++) {
      if (tolower((unsigned char)haystack[i + j]) !=
          tolower((unsigned char)needle[j]))
        break;
    }
    if (j == nlen) return 1;
  }
  return 0;
}

static void cmd_search(const char *term) {
  int w_front = (int)strlen("Front");
  int w_back  = (int)strlen("Back");
  for (int i = 0; i < n_cards; i++) {
    if (!contains_icase(cards[i].front, term) &&
        !contains_icase(cards[i].back, term))
      continue;
    int fl = (int)strlen(cards[i].front);
    int bl = (int)strlen(cards[i].back);
    if (fl > w_front) w_front = fl;
    if (bl > w_back)  w_back  = bl;
  }
  if (w_front > 40) w_front = 40;
  if (w_back  > 40) w_back  = 40;

  long today = today_day();
  int found = 0;
  for (int i = 0; i < n_cards; i++) {
    if (!contains_icase(cards[i].front, term) &&
        !contains_icase(cards[i].back, term))
      continue;

    if (found == 0) {
      printf(C_BOLD "%-*s  %-*s  %-10s  %8s  %4s  %4s\n" C_RESET,
             w_front, "Front", w_back, "Back", "Next Review", "Interval", "Reps", "EF");
      int sep_len = w_front + 2 + w_back + 2 + 10 + 2 + 8 + 2 + 4 + 2 + 4;
      printf(C_DIM);
      for (int s = 0; s < sep_len; s++) putchar('-');
      printf(C_RESET "\n");
    }
    found++;

    Progress *p = find_progress(cards[i].front);
    char date_buf[16] = "new";
    int interval = 0, reps = 0;
    float ef = 0.0f;
    if (p) {
      format_date(p->next_day, date_buf, sizeof(date_buf));
      interval = p->interval;
      reps     = p->reps;
      ef       = p->ef;
    }
    const char *row_color  = "";
    const char *date_color = "";
    if (!p) {
      date_color = C_BLUE;
    } else if (p->next_day <= today) {
      row_color  = C_YELLOW;
      date_color = C_YELLOW C_BOLD;
    } else {
      date_color = C_GREEN;
    }
    printf("%s%-*.*s  %-*.*s  %s%-10s" C_RESET "%s  %8d  %4d  %4.2f\n" C_RESET,
           row_color,
           w_front, w_front, cards[i].front,
           w_back,  w_back,  cards[i].back,
           date_color, date_buf,
           row_color, interval, reps, ef);
  }

  if (found == 0)
    printf(C_DIM "No cards matching \"%s\".\n" C_RESET, term);
  else
    printf(C_DIM "\n%d card(s) found.\n" C_RESET, found);
}

static int is_command(const char *s) {
  return strcmp(s, "--add") == 0 || strcmp(s, "--review") == 0 ||
         strcmp(s, "--show") == 0 || strcmp(s, "--stats") == 0 ||
         strcmp(s, "--search") == 0 || strcmp(s, "--help") == 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2 || strcmp(argv[1], "--help") == 0 ||
      (argc >= 3 && strcmp(argv[2], "--help") == 0)) {
    print_usage(argv[0]);
    return 0;
  }

  const char *cmd = NULL;
  const char *search_term = NULL;
  int reverse = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--reverse") == 0) {
      reverse = 1;
    } else if (strcmp(argv[i], "--search") == 0) {
      cmd = "--search";
      if (i + 1 < argc && !is_command(argv[i + 1]) &&
          strcmp(argv[i + 1], "--reverse") != 0)
        search_term = argv[++i];
    } else if (is_command(argv[i])) {
      cmd = argv[i];
    } else if (n_files < MAX_FILES) {
      snprintf(g_cards_files[n_files++], 4096, "%s", argv[i]);
    }
  }

  if (!cmd) {
    fprintf(stderr, "Missing command.\n\n");
    print_usage(argv[0]);
    return 1;
  }
  if (strcmp(cmd, "--search") == 0 && !search_term) {
    fprintf(stderr, "--search requires a search term.\n\n");
    print_usage(argv[0]);
    return 1;
  }
  if (strcmp(cmd, "--show") != 0 && strcmp(cmd, "--review") != 0 &&
      strcmp(cmd, "--add") != 0 && strcmp(cmd, "--stats") != 0 &&
      strcmp(cmd, "--search") != 0) {
    fprintf(stderr, "Unknown command: %s\n\n", cmd);
    print_usage(argv[0]);
    return 1;
  }
  if (n_files == 0) {
    fprintf(stderr, "Missing cards file.\n\n");
    print_usage(argv[0]);
    return 1;
  }

  if (strcmp(cmd, "--add") == 0) {
    if (n_files != 1) {
      fprintf(stderr, "--add requires exactly one cards file.\n");
      return 1;
    }
    return cmd_add(g_cards_files[0]) < 0 ? 1 : 0;
  }

  for (int fi = 0; fi < n_files; fi++) {
    snprintf(g_progress_files[fi], 4096, "%s%s.progress",
             g_cards_files[fi], reverse ? ".rev" : "");
    if (load_cards(g_cards_files[fi], fi) < 0)
      return 1;
    load_progress(g_progress_files[fi]);
  }
  if (n_cards == 0) {
    fprintf(stderr, "No cards found.\n");
    return 1;
  }

  if (reverse) {
    for (int i = 0; i < n_cards; i++) {
      char tmp[MAX_FIELD];
      snprintf(tmp, MAX_FIELD, "%s", cards[i].front);
      snprintf(cards[i].front, MAX_FIELD, "%s", cards[i].back);
      snprintf(cards[i].back, MAX_FIELD, "%s", tmp);
    }
  }

  srand((unsigned int)time(NULL));
  shuffle_cards();

  if (strcmp(cmd, "--show") == 0) {
    cmd_show();
  } else if (strcmp(cmd, "--stats") == 0) {
    cmd_stats();
  } else if (strcmp(cmd, "--search") == 0) {
    cmd_search(search_term);
  } else {
    cmd_review();
  }

  return 0;
}
