//---------------------------------------------------------------------
// RPi LEDs
#define PI_L_RED   27
#define PI_L_BLUE  17
#define PI_L_GREEN 22

// RPi buttons
#define PI_B_RED   24
#define PI_B_BLUE  25
#define PI_B_GREEN 23

#define PI_B_UP     5
#define PI_B_DOWN  13
#define PI_B_RIGHT  6
#define PI_B_LEFT  19

//---------------------------------------------------------------------
// Paths
#define PATH_LGHS_LOGO  "/home/asciimaton/logo.mpg"
#define PATH_FRAME_BIN  "/tmp/frame.bin"
#define PATH_FRAME_TXT  "/tmp/frame.txt"
#define PATH_FRAME_PGM  "/tmp/frame.pgm"
#define PATH_FRAME_SAV  "/home/asciimaton/photos/camera_%Y%m%d_%H%M%S.txt"
#define PATH_DEV_LP     "/dev/usb/lp0"
#define PATH_DEV_FB     "/dev/fb0"

//---------------------------------------------------------------------
#define CAM_WIDTH       264
#define CAM_HEIGHT      192

// flipped clockwise, saved to frame.bin
#define BIN_WIDTH       CAM_HEIGHT
#define BIN_HEIGHT      CAM_WIDTH

//---------------------------------------------------------------------
#define TXT_FACTOR      ((float) IMG2TXT_PIXNUM)
#define BRIGHTNESS      0.1
#define CONTRAST        1.7

//---------------------------------------------------------------------
// Repeat lines <n> times while printing for a stronger print.
#define REPEAT_LINES    2

//---------------------------------------------------------------------
// The footer may contain newlines (\n)
#define FOOTER          "asciimaton"

//---------------------------------------------------------------------
#define USEC_SLEEP         ( 10*1000)

#define LIVE_TIMEOUT       (180*1000)
#define STILL_TIMEOUT      (120*1000)
#define DEBOUNCE_THRESHOLD (      50)
#define TOGGLE_THRESHOLD   (     250)
