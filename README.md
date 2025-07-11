![Banner](https://s-christy.com/sbs/status-banner.svg?icon=social/school&hue=120&title=Spaced%20Repetition&description=SM-2%20flashcard%20review%20from%20the%20command%20line)

## Overview

This is a command-line spaced repetition tool written in C. It implements the
SM-2 algorithm to schedule flashcard reviews at optimal intervals, helping you
retain information with the minimum number of review sessions. Cards are stored
in plain TSV files and progress is tracked in companion `.progress` files.

The SM-2 algorithm adjusts the review interval for each card based on how well
you recalled it, using a score from 0 (complete blackout) to 5 (perfect
recall).  Cards answered poorly are scheduled sooner; cards answered
confidently are pushed further out. An ease factor per card controls how
aggressively the interval grows over time.

## Features

- Implements the SM-2 spaced repetition algorithm
- Tab-separated value (TSV) card format - easy to edit by hand or generate programmatically
- Progress persisted to sidecar `.progress` files alongside each deck
- Color-coded terminal output for at-a-glance status
- Review mode that only shows cards due today
- Table view showing each card's next review date, interval, reps, and ease factor
- Deck statistics: total cards, new, due today, due this week, average ease factor
- Case-insensitive substring search across all loaded decks
- Interactive card-add mode to append new cards without leaving the terminal
- `--reverse` flag to drill cards back-to-front with separate progress tracking
- Multiple deck files can be loaded in a single session

## Usage

## Card Format

## Dependencies

## License

This work is licensed under the GNU General Public License version 3 (GPLv3).

[<img src="https://s-christy.com/status-banner-service/GPLv3_Logo.svg" width="150" />](https://www.gnu.org/licenses/gpl-3.0.en.html)
