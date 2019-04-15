/*
 * Copyright (c) 2019 Sanggu Han
 */

#include "modes.h"
#include "message.h"
#include "operation.h"

#define MODE_FUNCTION(num, ...) \
	void mode##num(int input)\
	__VA_ARGS__

/* flags for alarm handler */
static int sec;
static int alarm_flag;
/* variables for mode 3 */
static char text_pad[9][4] = {
	".QZ", "ABC", "DEF", "GHI", "JKL",
	"MNO", "PRS", "TUV", "WXY"
};
static int prev, cnt, numeric;
/* variables for mode 5 */
static int ingame, ans;

/* set function pointer array w/ mode functions */
static mode_func_ptr modes[5] = {
	mode1, mode2, mode3, mode4, mode5
};

/**
 * periodic_control - alarm signal handler for periodic output
 */
void periodic_control(int signo) {
	int r;
	unsigned char ans_led = 0;

	if (alarm_flag & FLAG_MODE_1) {
		if (alarm_flag & FLAG_BLINK) {
			control_led(LED_SET, 1 << (5 - !sec));
			sec ^= 1;
			alarm(1);
		} else if (alarm_flag & FLAG_BOARD_TIME) {
			control_fnd(FND_SET_BOARD_TIME);
			alarm(5);
		} else {
			sec += 5;
			if (sec == 60) {
				control_fnd(FND_ADD_MINUTE);
				sec = 0;
			}
			alarm(5);
		}
	}

	if (alarm_flag & FLAG_MODE_4) {
		control_dot(DOT_CURSOR_SHOW);
		alarm(1);
	}

	if (alarm_flag & FLAG_MODE_5) {
		if (sec) {
			sec -= 1;
			control_dot(DOT_PRINT_1 + sec);
			alarm(1);
		} else {
			control_dot(DOT_RESET);
			control_dot(DOT_REVERSE);
			srand(time(NULL));
			do {
				r = rand() % 8;
				ans |= 1 << r;
				ans_led |= 1 << (7 - r);
			} while (r % 2);
			control_led(LED_SET, ans_led);
		}
	}
}

/**
 * initialize_board - initialize board states
 */
void initialize_board() {
	alarm(0);
	alarm_flag = sec = 0;
	prev = -1;
	numeric = cnt = 0;
	ingame = ans = 0;
	control_fnd(FND_RESET);
	control_lcd(LCD_RESET, 0);
	control_dot(DOT_RESET);
	control_led(LED_RESET, 0);
}

/**
 * initiate_mode - set initial state for given mode
 */
void initiate_mode(int mode) {
        switch (mode) {
        case 0:
                /*
                 * mode 1 - clock
                 */
		control_fnd(FND_SET_BOARD_TIME);
		control_led(LED_SET, 1 << 7);
		alarm_flag = FLAG_MODE_1 | FLAG_BOARD_TIME;
		alarm(5);
                break;
        case 1:
                /*
                 * mode 2 - counter
                 */
		control_led(LED_SET, 1 << 6);
                break;
        case 2:
                /*
                 * mode 3 - text editor
                 */
		control_dot(DOT_PRINT_INPUT_MODE);
                break;
        case 3:
                /*
                 * mode 4 - draw board
                 */
		control_dot(DOT_CURSOR_SHOW);
		alarm_flag = FLAG_MODE_4;
		alarm(1);
                break;
        case 4:
                /*
                 * mode 5 - additional function
                 */
                break;
        default:
                break;
        }
}

void run_mode(int mode, int input) {
	modes[mode](input);
}

MODE_FUNCTION(1, {
	switch (input) {
	case SW_1:
		if (alarm_flag & FLAG_BOARD_TIME) {
			alarm_flag ^= FLAG_BOARD_TIME;
		}
		alarm_flag ^= FLAG_BLINK;
		control_led(LED_SET, alarm_flag & FLAG_BLINK ? 1 << 5 : 1 << 7);
		sec = 0;
		alarm(1);
		break;
	case SW_2:
		if (alarm_flag & FLAG_BLINK) {
			control_fnd(FND_SET_BOARD_TIME);
		}
		break;
	case SW_3:
		if (alarm_flag & FLAG_BLINK) {
			control_fnd(FND_ADD_HOUR);
		}
		break;
	case SW_4:
		if (alarm_flag & FLAG_BLINK) {
			control_fnd(FND_ADD_MINUTE);
		}
		break;
	default:
		break;
	}
})

MODE_FUNCTION(2, {
	int base;

	switch (input) {
	case SW_1:
		control_fnd(FND_CHANGE_BASE);
		break;
	case SW_2:
		control_fnd(FND_ADD_SQUARE);
		break;
	case SW_3:
		control_fnd(FND_ADD_BASE);
		break;
	case SW_4:
		control_fnd(FND_INCREASE);
		break;
	default:
		break;
	}

	base = get_fnd_base();
	control_led(LED_SET, 1 << (8 - base));
})

MODE_FUNCTION(3, {
	int cur, i;

	control_fnd(FND_INCREASE);

	switch (input) {
	case SW_1:
	case SW_2:
	case SW_3:
	case SW_4:
	case SW_5:
	case SW_6:
	case SW_7:
	case SW_8:
	case SW_9:
		for (i = 0; i < 9; i++) {
			if ((input >> i) & 1) {
				cur = i;
			}
		}
		if (numeric) {
			control_lcd(LCD_ADD_CHAR, '1' + cur);
		} else {
			if (prev == cur) {
				cnt = (cnt + 1) % 3;
				control_lcd(LCD_REPLACE, text_pad[cur][cnt]);
			} else {
				cnt = 0;
				control_lcd(LCD_ADD_CHAR, text_pad[cur][cnt]);
			}
			prev = cur;
		}
		break;
	case SW_2 | SW_3:
		prev = -1;
		control_lcd(LCD_RESET, 0);
		break;
	case SW_5 | SW_6:
		numeric ^= 1;
		control_dot(DOT_PRINT_INPUT_MODE);
		break;
	case SW_8 | SW_9:
		control_lcd(LCD_ADD_CHAR, ' ');
		break;
	default:
		break;
	}
})

MODE_FUNCTION(4, {
	/* variable for loop counter */
	int i;

	control_fnd(FND_INCREASE);

	switch (input) {
	case SW_1:
	case SW_2:
	case SW_4:
	case SW_5:
	case SW_6:
	case SW_7:
	case SW_8:
	case SW_9:
		for (i = 0; i < 9; i++) {
			if ((input >> i) & 1) {
				control_dot(i + 1);
			}
		}
		break;
	case SW_3:
		alarm_flag ^= FLAG_MODE_4;
		if (alarm_flag) {
			control_dot(DOT_CURSOR_SHOW);
			alarm(1);
		} else {
			control_dot(DOT_CURSOR_HIDE);
		}
		break;
	default:
		break;
	}
})

MODE_FUNCTION(5, {
	if (ingame) {
		control_fnd(FND_INCREASE);
	}

	switch (input) {
	case SW_5:
		if (!ingame) {
			ingame = 1;
			control_dot(DOT_PRINT_3);
			sec = 2;
			alarm_flag |= FLAG_MODE_5;
			alarm(1);
		}
		break;
	case SW_9:
		ans = 0;
		ingame = 0;
		control_led(LED_RESET, 0);
		control_dot(DOT_RESET);
		control_lcd(LCD_RESET, 0);
		control_fnd(FND_RESET);
		break;
	default:
		break;
	}

	if (ingame && input == ans) {
		control_dot(DOT_REVERSE);
		control_lcd(LCD_ADD_CHAR, 'C');
		control_lcd(LCD_ADD_CHAR, 'O');
		control_lcd(LCD_ADD_CHAR, 'N');
		control_lcd(LCD_ADD_CHAR, 'G');
		control_lcd(LCD_ADD_CHAR, 'R');
		control_lcd(LCD_ADD_CHAR, 'A');
		control_lcd(LCD_ADD_CHAR, 'T');
		control_lcd(LCD_ADD_CHAR, 'S');
		control_lcd(LCD_ADD_CHAR, '!');
	}
})
