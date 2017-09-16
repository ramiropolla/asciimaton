// Copyright: Ramiro Polla
// License: GPLv3

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

extern "C" {
#include "algo_img2txt.h"
#include "algo_txt2img.h"
#include "pgm.h"
#include "txt.h"
#include "io.h"
}
#include "img2txt.h"
#include "txt2img.h"

#include "lp.h"
#include "cp.h"

#include <gst/gst.h>

#include "config.h"

//---------------------------------------------------------------------
// GPIO
static int gpio = -1;

//---------------------------------------------------------------------
static void change_state(int next);

//---------------------------------------------------------------------
static double brightness = BRIGHTNESS;
static double contrast = CONTRAST;

//---------------------------------------------------------------------
static uint64_t millis(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	uint64_t t = now.tv_nsec;
	t /= 1000000;
	t += now.tv_sec * 1000;

	return t;
}

//---------------------------------------------------------------------
enum e_states
{
	BAD = 0,
	LOGO,
	LIVE,
	SNAP,
	STILL,
	PRINT,
	N_STATES,
};

//---------------------------------------------------------------------
enum e_leds
{
	RED = 0,
	BLUE,
	GREEN,
	N_LEDS,
};

//---------------------------------------------------------------------
enum e_sigs
{
	B_RED = 0,
	B_BLUE,
	B_GREEN,
	B_UP,
	B_DOWN,
	B_RIGHT,
	B_LEFT,
	N_SIGS,
};

//---------------------------------------------------------------------
static void clear_fb(void)
{
	const char zeros[1024] = { 0 };
	FILE *fp = fopen(PATH_DEV_FB, "wb");
	if ( fp != NULL )
	{
		while ( fwrite(zeros, sizeof(zeros), 1, fp) != 0 )
			;
		fclose(fp);
	}
}

#pragma GCC diagnostic ignored "-Wunused-result"

//---------------------------------------------------------------------
static uint8_t *bin_read(const char *fname)
{
	FILE *fp_in = fopen(fname, "rb");
	if ( fp_in == NULL )
		return NULL;
	fseek(fp_in, 0, SEEK_END);
	size_t in_size = ftell(fp_in);
	fseek(fp_in, 0, SEEK_SET);
	uint8_t *buf = (uint8_t *) malloc(in_size);
	(void) fread(buf, in_size, 1, fp_in);
	fclose(fp_in);
	return buf;
}

//---------------------------------------------------------------------
// state: logo
static GstElement *logo_pipeline;
static void logo_init(int next)
{
	GError *error = NULL;
	logo_pipeline = gst_parse_launch(
	    "multifilesrc location=" PATH_LGHS_LOGO " loop=true ! "
	    "tsdemux ! mpeg2dec ! videoconvert ! "
	    "videoflip method=clockwise ! fbdevsink", &error);
	clear_fb();
	gst_element_set_state(logo_pipeline, GST_STATE_PLAYING);
}
static void logo_term(int next)
{
	gst_element_set_state(logo_pipeline, GST_STATE_NULL);
	gst_object_unref(logo_pipeline);
	logo_pipeline = NULL;
}

//---------------------------------------------------------------------
// state: live
static GstElement *live_pipeline;
static GstElement *live_balance;
static void live_init(int next)
{
	char buf[0x1000];
	sprintf(buf,
	    "v4l2src device=/dev/video0 ! "
	    "video/x-raw,format=I420,width=320,height=240,pixel-aspect-ratio=1/1,framerate=10/1 ! "
	    "videoflip method=horizontal-flip ! "
	    "videocrop left=28 right=28 top=24 bottom=24 ! "
	    "queue ! "
	    "videobalance name=balance brightness=1 contrast=1 ! "
	    "videoconvert ! "
	    "video/x-raw,format=GRAY8,width=%d,height=%d,pixel-aspect-ratio=1/1 ! "
	    "tee name=t ! "
	    "queue ! videoflip method=counterclockwise ! "
	    "multifilesink location=%s "
	    "t. ! queue ! videoconvert ! "
	    "queue ! "
	    "videoscale add-borders=false ! "
	    "video/x-raw,format=RGB,width=1386,height=1008,pixel-aspect-ratio=1/1 ! "
	    "queue ! "
	    "videoscale ! "
	    "queue ! "
	    "videoconvert ! "
	    "fbdevsink", CAM_WIDTH, CAM_HEIGHT, PATH_FRAME_BIN);
	GError *error = NULL;
	live_pipeline = gst_parse_launch(buf, &error);
	live_balance = gst_bin_get_by_name((GstBin *) live_pipeline, "balance");
	g_object_set(live_balance, "contrast", contrast, NULL);
	g_object_set(live_balance, "brightness", brightness, NULL);
	gst_element_set_state(live_pipeline, GST_STATE_PLAYING);
}
static void kill_live(void)
{
	fprintf(stderr, "%s\n", __func__);

	gst_object_unref(live_balance);
	live_balance = NULL;

	gst_element_set_state(live_pipeline, GST_STATE_NULL);
	gst_object_unref(live_pipeline);
	live_pipeline = NULL;
}
static void live_term(int next)
{
	if ( next == LOGO )
		kill_live();
}

//---------------------------------------------------------------------
// state: snap
static void count_leds(void);
static void snap_init(int next)
{
	count_leds();
	change_state(STILL);
}
static void snap_term(int next)
{
	kill_live();
}

//---------------------------------------------------------------------
// state: still
static GstElement *still_pipeline;
static void still_init(int next)
{
	uint8_t *bin_buf = NULL;
	char *txt_buf = NULL;
	uint8_t *pgm_buf = NULL;
	size_t pgm_w;
	size_t pgm_h;
	size_t txt_w;
	size_t txt_h;

	bin_buf = bin_read(PATH_FRAME_BIN);
	txt_buf = img2txt(bin_buf, BIN_WIDTH, BIN_HEIGHT, &txt_w, &txt_h, TXT_FACTOR, FOOTER);
	pgm_buf = txt2img(txt_buf, txt_w, txt_h, &pgm_w, &pgm_h);

	txt_write(PATH_FRAME_TXT, txt_buf);
	pgm_write(PATH_FRAME_PGM, pgm_buf, pgm_w, pgm_h);

	free(bin_buf);
	free(txt_buf);
	free(pgm_buf);

	GError *error = NULL;
	still_pipeline = gst_parse_launch(
	    "multifilesrc location=" PATH_FRAME_PGM " loop=true ! "
	    "pnmdec ! "
	    "videoflip method=clockwise ! "
	    "videoconvert ! "
	    "videoscale add-borders=false ! "
	    "video/x-raw,format=RGB,width=1386,height=1008,pixel-aspect-ratio=1/1 ! "
	    "videoscale ! "
	    "videoconvert ! "
	    "fbdevsink", &error);
	gst_element_set_state(still_pipeline, GST_STATE_PLAYING);
}
static void still_term(int next)
{
	gst_element_set_state(still_pipeline, GST_STATE_NULL);
	gst_object_unref(still_pipeline);
	still_pipeline = NULL;
}

//---------------------------------------------------------------------
// state: print
static void print_init(int next)
{
	char fname[0x100];
	struct tm *ttime;
	time_t now = time(NULL);
	ttime = gmtime(&now);
	strftime(fname, sizeof(fname), PATH_FRAME_SAV, ttime);
	fprintf(stderr, "copying [%s] to [%s]\n", PATH_FRAME_TXT, fname);
	copy_text_file(PATH_FRAME_TXT, fname);

	fprintf(stderr, "printing [%s]\n", fname);
	print(fname, PATH_DEV_LP, REPEAT_LINES);

	usleep(1000000);
	change_state(STILL);
}
static void print_term(int next)
{
	usleep(1000000);
}

//---------------------------------------------------------------------
struct state_t
{
	const char *name;

	void (*init)(int next);
	void (*term)(int next);

	int sig[N_SIGS];

	int timeout;

	unsigned int timeout_val;
};

//---------------------------------------------------------------------
static struct state_t states[N_STATES] = {
	//          name     init()      term()        red   blue   green    timeout
	[BAD  ] = { "",      NULL,       NULL,       { 0,    0,     0,    }, 0                   },
	[LOGO ] = { "logo",  logo_init,  logo_term,  { 0,    0,     LIVE, }, 0,                  },
	[LIVE ] = { "live",  live_init,  live_term,  { LOGO, 0,     SNAP, }, LOGO, LIVE_TIMEOUT  },
	[SNAP ] = { "snap",  snap_init,  snap_term,  { 0,    0,     0,    }, 0,                  }, // seq: still
	[STILL] = { "still", still_init, still_term, { LIVE, PRINT, 0,    }, LIVE, STILL_TIMEOUT },
	[PRINT] = { "print", print_init, print_term, { 0,    0,     0,    }, 0,                  }, // seq: live
};
static struct state_t *state = &states[BAD];

static void change_state(int next)
{
	if ( state->term != NULL )
	{
		fprintf(stderr, "%s_term\n", state->name);
		state->term(next);
	}
	state = &states[next];
	if ( state->init != NULL )
	{
		fprintf(stderr, "%s_init\n", state->name);
		state->init(next);
	}
}

//---------------------------------------------------------------------
static int l_bda2bcm[N_LEDS] = {
	[RED  ] = PI_L_RED,
	[BLUE ] = PI_L_BLUE,
	[GREEN] = PI_L_GREEN,
};

//---------------------------------------------------------------------
struct leds_t
{
	leds_t()
	{
		if ( gpio == -1 )
			gpio = io_start();
		for ( int i = 0; i < N_LEDS; i++ )
		{
			io_set_output(gpio, l_bda2bcm[i]);
			status[i] = false;
		}
	}

	bool status[N_LEDS];
	void set(int n, bool _status)
	{
		status[n] = _status;
		io_write(gpio, l_bda2bcm[n], status[n]);
		fprintf(stderr, "LED %d is %d\n", n, status[n]);
	}
	void toggle(int n)
	{
		set(n, !status[n]);
	}
};
static leds_t leds;

//---------------------------------------------------------------------
static void count_leds(void)
{
	leds.set(GREEN, false);
	leds.set(RED, false);
	leds.set(BLUE, false);
	usleep(1000000);
	leds.set(GREEN, true);
	usleep(1000000);
	leds.set(RED, true);
	usleep(1000000);
	leds.set(BLUE, true);
	usleep(1000000);
}

//---------------------------------------------------------------------
static int b_bda2bcm[N_SIGS] = {
	[B_RED  ] = PI_B_RED,
	[B_BLUE ] = PI_B_BLUE,
	[B_GREEN] = PI_B_GREEN,
	[B_UP   ] = PI_B_UP,
	[B_DOWN ] = PI_B_DOWN,
	[B_RIGHT] = PI_B_RIGHT,
	[B_LEFT ] = PI_B_LEFT,
};

//---------------------------------------------------------------------
class buttons_t
{
public:
	buttons_t()
	{
		if ( gpio == -1 )
			gpio = io_start();
		for ( int i = 0; i < N_SIGS; i++ )
		{
			io_set_input(gpio, b_bda2bcm[i]);
			io_set_pull_up_down(gpio, b_bda2bcm[i]);
		}
		reset();
	}

	bool pressed[N_SIGS];

	void read()
	{
		// read buttons
		io_update();
		bool temp_pressed[N_SIGS];
		for ( int i = 0; i < N_SIGS; i++ )
			temp_pressed[i] = io_read(gpio, b_bda2bcm[i]);

		// debounce buttons into this->pressed
		uint64_t cur_time = millis();
		for ( int i = 0; i < N_SIGS; i++ )
		{
			if ( !temp_pressed[i] )
			{
				debounce[i] = 0;
				continue;
			}
			if ( debounce[i] == 0 )
				debounce[i] = cur_time;
			if ( cur_time - debounce[i] >= DEBOUNCE_THRESHOLD )
				pressed[i] = true;
		}

		// blink LEDs
		while ( cur_time - toggle_time > TOGGLE_THRESHOLD )
		{
			toggle_time += TOGGLE_THRESHOLD;
			for ( int i = 0; i < N_LEDS; i++ )
				if ( state->sig[i] != 0 )
					leds.toggle(i);
		}
	}

	void reset()
	{
		// clear all buttons
		for ( int i = 0; i < N_SIGS; i++ )
		{
			pressed[i] = false;
			debounce[i] = 0;
		}

		// set all leds
		int count = 0;
		for ( int i = 0; i < N_LEDS; i++ )
		{
			if ( state->sig[i] != 0 )
				leds.set(i, (count++ & 1) != 0);
			else
				leds.set(i, false);
		}

		// start counting time
		start_time = millis();
		toggle_time = start_time;
	}

private:
	uint64_t start_time;
	uint64_t toggle_time;
	uint64_t debounce[N_SIGS];
};
static buttons_t buttons;

//---------------------------------------------------------------------
static void change_brightness(double inc)
{
	brightness += inc;
	if ( brightness > 1.0 )
		brightness = 1.0;
	if ( brightness < -1.0 )
		brightness = -1.0;
	fprintf(stderr, "brightness: %g\n", brightness);
	if ( live_balance != NULL )
		g_object_set(live_balance, "brightness", brightness, NULL);
}

static void change_contrast(double inc)
{
	contrast += inc;
	if ( contrast > 2.0 )
		contrast = 2.0;
	if ( contrast < 0.0 )
		contrast = 0.0;
	fprintf(stderr, "contrast: %g\n", contrast);
	if ( live_balance != NULL )
		g_object_set(live_balance, "contrast", contrast, NULL);
}

//---------------------------------------------------------------------
static void
main_loop(void)
{
	uint64_t t1 = millis();
	while ( 42 )
	{
		usleep(USEC_SLEEP);
		buttons.read();
		if ( state->timeout != 0 && (millis() - t1) > state->timeout_val )
		{
			change_state(state->timeout);
		}
		else
		{
			int i = 0;
			for ( ; i < N_LEDS; i++ )
			{
				if ( state->sig[i] != 0 && buttons.pressed[i] )
				{
					change_state(state->sig[i]);
					break;
				}
			}
			if ( i == N_LEDS )
			{
				// joystick
				for ( ; i < N_SIGS; i++ )
				{
					if ( buttons.pressed[i] )
					{
						switch ( i )
						{
							case B_UP:
								change_brightness(+0.05);
								break;
							case B_DOWN:
								change_brightness(-0.05);
								break;
							case B_RIGHT:
								change_contrast(+0.1);
								break;
							case B_LEFT:
								change_contrast(-0.1);
								break;
						}
					}
					buttons.pressed[i] = false;
				}
				continue;
			}
		}
		// state has changed
		buttons.reset();
		t1 = millis();
	}
}

//---------------------------------------------------------------------
int main(int argc, char *argv[])
{
	img2txt_fix_weights();
	gst_init(&argc, &argv);
	change_state(LOGO);
	main_loop();
	return 0;
}
