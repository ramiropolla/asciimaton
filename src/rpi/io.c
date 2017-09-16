// Copyright: Ramiro Polla
// License: WTFPL

#include "config.h"

// defined in the makefile
#ifdef RPI_GPIO

#include <pigpiod_if2.h>
int io_read(int pi, unsigned gpio)
{
	return gpio_read(pi, gpio);
}
int io_write(int pi, unsigned gpio, unsigned level)
{
	return gpio_write(pi, gpio, level);
}
int io_start(void)
{
	return pigpio_start(NULL, NULL);
}
void io_update(void)
{
}
void io_set_input(int pi, unsigned gpio)
{
	set_mode(pi, gpio, PI_INPUT);
}
void io_set_output(int pi, unsigned gpio)
{
	set_mode(pi, gpio, PI_OUTPUT);
}
void io_set_pull_up_down(int pi, unsigned gpio);
{
	set_pull_up_down(pi, gpio, PI_PUD_DOWN);
}

#else

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static struct termios orig_termios;
static char io_c;

static void reset_terminal_mode(void)
{
	tcsetattr(0, TCSANOW, &orig_termios);
}
static void set_conio_terminal_mode(void)
{
	struct termios new_termios;

	/* take two copies - one for now, one for later */
	tcgetattr(0, &orig_termios);
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	/* register cleanup handler, and set the new terminal mode */
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	new_termios.c_oflag |= OPOST;
	new_termios.c_lflag |= ISIG;
	tcsetattr(0, TCSANOW, &new_termios);
}
static int kbhit(void)
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}
static int getch(void)
{
	int r;
	unsigned char c;
	if ((r = read(0, &c, sizeof(c))) < 0) {
		return r;
	} else {
		return c;
	}
}
static int getkey(int pin)
{
	switch ( io_c )
	{
	case 'r': // red
		return !(pin == PI_B_RED);
	case 'g': // green
		return !(pin == PI_B_GREEN);
	case 'b': // blue
		return !(pin == PI_B_BLUE);
	case 'u': // up
		return !(pin == PI_B_UP);
	case 'd': // down
		return !(pin == PI_B_DOWN);
	case 'i': // right
		return !(pin == PI_B_RIGHT);
	case 'l': // left
		return !(pin == PI_B_LEFT);
	}
	return 0;
}
int io_read(int pi, unsigned gpio)
{
	return !getkey(gpio);
}
int io_write(int pi, unsigned gpio, unsigned level)
{
	return 0;
}
int io_start(void)
{
	set_conio_terminal_mode();
	return 0;
}
void io_update(void)
{
	io_c = (kbhit() == 0) ? 0 : getch();
}
void io_set_input(int pi, unsigned gpio)
{
}
void io_set_output(int pi, unsigned gpio)
{
}
void io_set_pull_up_down(int pi, unsigned gpio)
{
}

#endif
