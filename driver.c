/**
 * @file
 * @author Hamik Mukelyan
 *
 * Drives a text-based Flappy Bird knock-off that is intended to run in an
 * 80 x 24 console.
 */

#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <limits.h>

//-------------------------------- Definitions --------------------------------

/**
 * Represents a vertical pipe through which Flappy The Bird is supposed to fly.
 */
typedef struct vpipe {

	/*
	 * The height of the opening of the pipe as a fraction of the height of the
	 * console window.
	 */
	float opening_height;

	/*
	 * Center of the pipe is at this column number (e.g. somewhere in [0, 79]).
	 * When the center + radius is negative then the pipe's center is rolled
	 * over to somewhere > the number of columns and the opening height is
	 * changed.
	 */
	int center;
} vpipe;

/** Represents Flappy the Bird. */
typedef struct flappy {
	/* Height of Flappy the Bird at the last up arrow press. */
	int h0;

	/* Time since last up arrow pressed. */
	int t;
} flappy;

//------------------------------ Global Constants -----------------------------

/** Gravitational acceleration constant */
const float GRAV = 0.05;

/** Initial velocity with up arrow press */
const float V0 = -0.5;

/** Number of rows in the console window. */
const int NUM_ROWS = 24;

/** Number of columns in the console window. */
const int NUM_COLS = 80;

/** Radius of each vertical pipe. */
const int PIPE_RADIUS = 3;

/** Width of the opening in each pipe. */
const int OPENING_WIDTH = 7;

/** Flappy stays in this column. */
const int FLAPPY_COL = 10;

/** Aiming for this many frames per second. */
const float TARGET_FPS = 24;

/** Amount of time the splash screen stays up. */
const float START_TIME_SEC = 3;

/** Length of the "progress bar" on the status screen. */
const int PROG_BAR_LEN = 76;

/** Row number at which the progress bar will show. */
const int PROG_BAR_ROW = 22;

const int SCORE_START_COL = 62;

//------------------------------ Global Variables -----------------------------

/** Frame number. */
int frame = 0;

/** Number of pipes that have been passed. */
int score = 0;

/** Number of digits in the score. */
int sdigs = 1;

/** Best score so far. */
int best_score = 0;

/** Number of digits in the best score. */
int bdigs = 1;

/** The vertical pipe obstacles. */
vpipe p1, p2;

//---------------------------------- Functions --------------------------------

/**
 * Converts the given char into a string.
 *
 * @param ch Char to convert to a string.
 * @param[out] str Receives 'ch' into a null-terminated C string. Assumes
 * str had 2 bytes allocated.
 */
void chtostr(char ch, char *str) {
	str[0] = ch;
	str[1] = '\0';
}

/**
 * "Moving" floor and ceiling are written into the window array.
 *
 * @param ceiling_row
 * @param floor_row
 * @param ch Char to use for the ceiling and floor.
 * @param spacing Between chars in the floor and ceiling
 * @param col_start Stagger the beginning of the floor and ceiling chars
 * by this much
 */
void draw_floor_and_ceiling(int ceiling_row, int floor_row,
		char ch, int spacing, int col_start) {
	char c[2];
	chtostr(ch, c);
	int i;
	for (i = col_start; i < NUM_COLS - 1; i += spacing) {
		if (i < SCORE_START_COL - sdigs - bdigs)
			mvprintw(ceiling_row, i, c);
		mvprintw(floor_row, i, c);
	}
}

/**
 * Updates the pipe center and opening height for each new frame. If the pipe
 * is sufficiently far off-screen to the left the center is wrapped around to
 * the right, at which time the opening height is changed.
 */
void pipe_refresh(vpipe *p) {

	// If pipe exits screen on the left then wrap it to the right side of the
	// screen.
	if(p->center + PIPE_RADIUS < 0) {
		p->center = NUM_COLS + PIPE_RADIUS;

		// Get an opening height fraction.
		p->opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;
		score++;
		if(sdigs == 1 && score > 9)
			sdigs++;
		else if(sdigs == 2 && score > 99)
			sdigs++;
	}
	p->center--;
}

/**
 * Gets the row number of the top or bottom of the opening in the given pipe.
 *
 * @param p The pipe obstacle.
 * @param top Should be 1 for the top, 0 for the bottom.
 *
 * @return Row number.
 */
int get_orow(vpipe p, int top) {
	return p.opening_height * (NUM_ROWS - 1) -
			(top ? 1 : -1) * OPENING_WIDTH / 2;
}

/**
 * Draws the given pipe on the window using 'vch' as the character for the
 * vertical part of the pipe and 'hch' as the character for the horizontal
 * part.
 *
 * @param p
 * @param vch Character for vertical part of pipe
 * @param hcht Character for horizontal part of top pipe
 * @param hchb Character for horizontal part of lower pipe
 * @param ceiling_row Start the pipe just below this
 * @param floor_row Star the pipe jut above this
 */
void draw_pipe(vpipe p, char vch, char hcht, char hchb,
		int ceiling_row, int floor_row) {
	int i, upper_terminus, lower_terminus;
	char c[2];

	// Draw vertical part of upper half of pipe.
	for(i = ceiling_row + 1; i < get_orow(p, 1); i++) {
		if ((p.center - PIPE_RADIUS) >= 0 &&
				(p.center - PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center - PIPE_RADIUS, c);
		}
		if ((p.center + PIPE_RADIUS) >= 0 &&
				(p.center + PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center + PIPE_RADIUS, c);
		}
	}
	upper_terminus = i;

	// Draw horizontal part of upper part of pipe.
	for (i = -PIPE_RADIUS; i <= PIPE_RADIUS; i++) {
		if ((p.center + i) >= 0 &&
				(p.center + i) < NUM_COLS - 1) {
			chtostr(hcht, c);
			mvprintw(upper_terminus, p.center + i, c);
		}
	}

	// Draw vertical part of lower half of pipe.
	for(i = floor_row - 1; i > get_orow(p, 0); i--) {
		if ((p.center - PIPE_RADIUS) >= 0 &&
				(p.center - PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center - PIPE_RADIUS, c);
		}
		if ((p.center + PIPE_RADIUS) >= 0 &&
				(p.center + PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center + PIPE_RADIUS, c);
		}
	}
	lower_terminus = i;

	// Draw horizontal part of lower part of pipe.
	for (i = -PIPE_RADIUS; i <= PIPE_RADIUS; i++) {
		if ((p.center + i) >= 0 &&
				(p.center + i) < NUM_COLS - 1) {
			chtostr(hchb, c);
			mvprintw(lower_terminus, p.center + i, c);
		}
	}
}

/**
 * Get Flappy's height along its parabolic arc.
 *
 * @param f Flappy!
 *
 * @return height as a row count
 */
int get_flappy_position(flappy f) {
	return f.h0 + V0 * f.t + 0.5 * GRAV * f.t * f.t;
}

/**
 * Returns true if Flappy crashed into a pipe.
 *
 * @param f Flappy!
 * @param p The vertical pipe obstacle.
 *
 * @return 1 if Flappy crashed, 0 otherwise.
 */
int crashed_into_pipe(flappy f, vpipe p) {
	if (FLAPPY_COL >= p.center - PIPE_RADIUS - 1 &&
			FLAPPY_COL <= p.center + PIPE_RADIUS + 1) {

		if (get_flappy_position(f) >= get_orow(p, 1)  + 1 &&
				get_flappy_position(f) <= get_orow(p, 0) - 1) {
			return 0;
		}
		else {
			return 1;
		}
	}
	return 0;
}

/**
 * Prints a failure screen asking the user to either play again or quit.
 *
 * @return 1 if the user wants to play again. Exits the program otherwise.
 */
int failure_screen() {
	char ch;
	clear();
	mvprintw(NUM_ROWS / 2 - 1, NUM_COLS / 2 - 22,
			"Flappy died :-(. <Enter> to flap, 'q' to quit.\n");
	refresh();
	timeout(-1); // Block until user enters something.
	ch = getch();
	switch(ch) {
	case 'q': // Quit.
		endwin();
		exit(0);
		break;
	default:
		if (score > best_score)
			best_score = score;
		if (bdigs == 1 && best_score > 9)
			bdigs++;
		else if(bdigs == 2 && best_score > 99)
			bdigs++;
		score = 0;
		sdigs = 1;
		return 1; // Restart game.
	}
	endwin();
	exit(0);
}

/**
 * Draws Flappy to the screen and shows death message if Flappy collides with
 * ceiling or floor. The user can continue to play or can exit if Flappy
 * dies.
 *
 * @param f Flappy the bird!
 *
 * @return 0 if Flappy was drawn as expected, 1 if the game should restart.
 */
int draw_flappy(flappy f) {
	char c[2];
	int h = get_flappy_position(f);

	// If Flappy crashed into the ceiling or the floor...
	if (h <= 0 || h >= NUM_ROWS - 1)
		return failure_screen();

	// If Flappy crashed into a pipe...
	if (crashed_into_pipe(f, p1) || crashed_into_pipe(f, p2)) {
		return failure_screen();
	}

	// If going down, don't flap
	if (GRAV * f.t + V0 > 0) {
		chtostr('\\', c);
		mvprintw(h, FLAPPY_COL - 1, c);
		mvprintw(h - 1, FLAPPY_COL - 2, c);
		chtostr('0', c);
		mvprintw(h, FLAPPY_COL, c);
		chtostr('/', c);
		mvprintw(h, FLAPPY_COL + 1, c);
		mvprintw(h - 1, FLAPPY_COL + 2, c);
	}

	// If going up, flap!
	else {
		// Left wing
		if (frame % 6 < 3) {
			chtostr('/', c);
			mvprintw(h, FLAPPY_COL - 1, c);
			mvprintw(h + 1, FLAPPY_COL - 2, c);
		}
		else {
			chtostr('\\', c);
			mvprintw(h, FLAPPY_COL - 1, c);
			mvprintw(h - 1, FLAPPY_COL - 2, c);
		}

		// Body
		chtostr('0', c);
		mvprintw(h, FLAPPY_COL, c);

		// Right wing
		if (frame % 6 < 3) {
			chtostr('\\', c);
			mvprintw(h, FLAPPY_COL + 1, c);
			mvprintw(h + 1, FLAPPY_COL + 2, c);
		}
		else {
			chtostr('/', c);
			mvprintw(h, FLAPPY_COL + 1, c);
			mvprintw(h - 1, FLAPPY_COL + 2, c);
		}
	}

	return 0;
}

/**
 * Print a splash screen and show a progress bar. NB the ASCII art was
 * generated by patorjk.com.
 */
void splash_screen() {
	int i;
	int r = NUM_ROWS / 2 - 6;
	int c = NUM_COLS / 2 - 22;

	// Print the title.
	mvprintw(r, c,     " ___ _                       ___ _        _ ");
	mvprintw(r + 1, c, "| __| |__ _ _ __ _ __ _  _  | _ |_)_ _ __| |");
	mvprintw(r + 2, c, "| _|| / _` | '_ \\ '_ \\ || | | _ \\ | '_/ _` |");
	mvprintw(r + 3, c, "|_| |_\\__,_| .__/ .__/\\_, | |___/_|_| \\__,_|");
	mvprintw(r + 4, c, "           |_|  |_|   |__/                  ");
	mvprintw(NUM_ROWS / 2 + 1, NUM_COLS / 2 - 10,
			"Press <up> to flap!");

	// Print the progress bar.
	mvprintw(PROG_BAR_ROW, NUM_COLS / 2 - PROG_BAR_LEN / 2 - 1, "[");
	mvprintw(PROG_BAR_ROW, NUM_COLS / 2 + PROG_BAR_LEN / 2, "]");
	refresh();
	for(i = 0; i < PROG_BAR_LEN; i++) {
		usleep(1000000 * START_TIME_SEC / (float) PROG_BAR_LEN);
		mvprintw(PROG_BAR_ROW, NUM_COLS / 2 - PROG_BAR_LEN / 2 + i, "=");
		refresh();
	}
	usleep(1000000 * 0.5);
}

//------------------------------------ Main -----------------------------------

int main()
{
	int leave_loop = 0;
	int ch;
	flappy f;
	int restart = 1;

	srand(time(NULL));

	// Initialize ncurses
	initscr();
	raw();					// Disable line buffering
	keypad(stdscr, TRUE);
	noecho();				// Don't echo() for getch
	curs_set(0);
	timeout(0);

	splash_screen();

	while(!leave_loop) {

		// If we're just starting a game then do some initializations.
		if (restart) {
			timeout(0); // Don't block on input.

			// Start the pipes just out of view on the right.
			p1.center = (int)(1.2 * (NUM_COLS - 1));
			p1.opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;
			p2.center = (int)(1.75 * (NUM_COLS - 1));
			p2.opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;

			// Initialize flappy
			f.h0 = NUM_ROWS / 2;
			f.t = 0;
			restart = 0;
		}

		usleep((unsigned int) (1000000 / TARGET_FPS));

		// Process keystrokes.
		ch = -1;
		ch = getch();
		switch (ch) {
		case 'q': // Quit.
			endwin();
			exit(0);
			break;
		case KEY_UP: // Give Flappy a boost!
			f.h0 = get_flappy_position(f);
			f.t = 0;
			break;
		default: // Let Flappy fall along his parabola.
			f.t++;
		}

		clear();

		// Print "moving" floor and ceiling
		draw_floor_and_ceiling(0, NUM_ROWS - 1, '/', 3, frame % 3);

		// Update pipe locations and draw them.
		draw_pipe(p1, '|', '=', '=', 0, NUM_ROWS - 1);
		draw_pipe(p2, '|', '=', '=', 0, NUM_ROWS - 1);
		pipe_refresh(&p1);
		pipe_refresh(&p2);

		// Draw Flappy. If Flappy crashed and user wants a restart...
		if(draw_flappy(f)) {
			restart = 1;
			continue; // ...then restart the game.
		}

		mvprintw(0, SCORE_START_COL - bdigs - sdigs,
				" Score: %d  Best: %d", score, best_score);

		// Display all the chars for this frame.
		refresh();
		frame++;
	}

	endwin();

	return 0;
}
