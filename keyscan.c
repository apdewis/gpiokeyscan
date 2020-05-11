#include <wiringPi.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define ROW0 24
#define ROW1 27
#define ROW2 25
#define ROW3 28
#define ROW4 29

#define COL0 1
#define COL1 2
#define COL2 3
#define COL3 4
#define COL4 5
#define COL5 6

#define COL6 23
#define COL7 26
#define COL8 22
#define COL9 21
#define COL10 30
#define COL11 31

#define COLUMNS 12
#define ROWS 5

unsigned char keyStat[COLUMNS][ROWS];
unsigned char keyNewStat[COLUMNS][ROWS];

int keyMap[ROWS][COLUMNS] = {
	{0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_BACKSPACE},
	{KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_BACKSLASH},
	{KEY_ESC, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_ENTER},
	{KEY_LEFTSHIFT, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_DOT, KEY_UP, KEY_SLASH},
	{KEY_LEFTCTRL, 0, KEY_MENU, KEY_LEFTALT, KEY_APOSTROPHE, KEY_SPACE, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_MINUS, KEY_LEFT, KEY_DOWN, KEY_RIGHT}
};


int fnKeyMap[ROWS][COLUMNS] = {
	{0, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_UNKNOWN},
	{KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN},
	{KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN},
	{KEY_LEFTSHIFT, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_PAGEUP, KEY_UNKNOWN},
	{KEY_LEFTCTRL, 0, KEY_MENU, KEY_LEFTALT, KEY_APOSTROPHE, KEY_SPACE, KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_EQUAL, KEY_HOME, KEY_DOWN, KEY_PAGEDOWN}
};
int rowGpio[] = { ROW0, ROW1, ROW2, ROW3, ROW4 };
int row = 0;

int tfd, epfd, ret;
struct epoll_event ev;
struct itimerspec ts;
static char *itimerspec_dump(struct itimerspec *ts);
struct uinput_setup usetup;
int uinputFd = 0;
long int res;

void setRow(unsigned char row) 
{
	unsigned char idx;
	for(idx = 0; idx < ROWS; idx++)
	{
		digitalWrite(rowGpio[idx], 1);	
	}
	
	digitalWrite(rowGpio[row], 0);	
}

void readRowValues()
{
	keyNewStat[0][row] = digitalRead(COL0);
	keyNewStat[1][row] = digitalRead(COL1);
	keyNewStat[2][row] = digitalRead(COL2);
	keyNewStat[3][row] = digitalRead(COL3);
	keyNewStat[4][row] = digitalRead(COL4);
	keyNewStat[5][row] = digitalRead(COL5);
	keyNewStat[6][row] = digitalRead(COL6);
	keyNewStat[7][row] = digitalRead(COL7);
	keyNewStat[8][row] = digitalRead(COL8);
	keyNewStat[9][row] = digitalRead(COL9);
	keyNewStat[10][row] = digitalRead(COL10);
	keyNewStat[11][row] = digitalRead(COL11);
}

void init() 
{
	wiringPiSetup();
	pinMode(COL0, INPUT);
	pullUpDnControl (COL0, PUD_UP);
	pinMode(COL1, INPUT);
	pullUpDnControl (COL1, PUD_UP);
	pinMode(COL2, INPUT);
	pullUpDnControl (COL2, PUD_UP);
	pinMode(COL3, INPUT);
	pullUpDnControl (COL3, PUD_UP);
	pinMode(COL4, INPUT);
	pullUpDnControl (COL4, PUD_UP);
	pinMode(COL5, INPUT);
	pullUpDnControl (COL5, PUD_UP);
	pinMode(COL6, INPUT);
	pullUpDnControl (COL6, PUD_UP);
	pinMode(COL7, INPUT);
	pullUpDnControl (COL7, PUD_UP);
	pinMode(COL8, INPUT);
	pullUpDnControl (COL8, PUD_UP);
	pinMode(COL9, INPUT);
	pullUpDnControl (COL9, PUD_UP);
	pinMode(COL10, INPUT);
	pullUpDnControl (COL10, PUD_UP);
	pinMode(COL11, INPUT);
	pullUpDnControl (COL11, PUD_UP);
	
	pinMode(ROW0, OUTPUT);	
	pinMode(ROW1, OUTPUT);	
	pinMode(ROW2, OUTPUT);	
	pinMode(ROW3, OUTPUT);	
	pinMode(ROW4, OUTPUT);
	digitalWrite(ROW0, 1);
	digitalWrite(ROW1, 1);
	digitalWrite(ROW2, 1);
	digitalWrite(ROW3, 1);
	digitalWrite(ROW4, 1);

	//init old keys
	for(int x = 0; x < COLUMNS; x++) 
	{
		for(int y = 0; y < ROWS; y++)
		{
			keyStat[x][y] = 1;
			keyNewStat[x][y] = 1;
		} 
	} 

	//setup uinput
	uinputFd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	ioctl(uinputFd, UI_SET_EVBIT, EV_KEY);
	for(unsigned char i; i < 249; i++)
	{
		ioctl(uinputFd, UI_SET_KEYBIT, i);
	}

	memset(&usetup, 0, sizeof(usetup));
   	usetup.id.bustype = BUS_USB;
   	usetup.id.vendor = 0x1234; /* sample vendor */
   	usetup.id.product = 0x5678; /* sample product */
   	strcpy(usetup.name, "Example device");

   	ioctl(uinputFd, UI_DEV_SETUP, &usetup);
   	ioctl(uinputFd, UI_DEV_CREATE);
	perror("bork");
}

void emit(int fd, int type, int code, int val)
{
   struct input_event ie;

   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   write(uinputFd, &ie, sizeof(ie));
}

int main () {
	init();

	while(1)
	{
		setRow(row);

		//seem to have to do this multiple times as a crude delay to get glitch free scans
		readRowValues();
		readRowValues();
		readRowValues();
		readRowValues();
		readRowValues();


		row++;
		if(row == ROWS)
		{
			for(int x = 0; x < COLUMNS; x++) 
			{
				for(int y = 0; y < ROWS; y++)
				{
					if(keyNewStat[x][y] == 0  && keyStat[x][y] == 1) 
					{
						if(keyNewStat[1][4] == 0)
						{
							emit(uinputFd, EV_KEY, fnKeyMap[y][x], 1);
						}
						else
						{
							emit(uinputFd, EV_KEY, keyMap[y][x], 1);
						}
						printf("keydown \n\r");
						printf("%d %d \n \r", x, y);
						emit(uinputFd, EV_SYN, SYN_REPORT, 0);
						keyStat[x][y] = 0;
					}
					else if(keyNewStat[x][y] == 1 && keyStat[x][y] == 0)
					{
						if(keyNewStat[1][4] == 0)
						{
							emit(uinputFd, EV_KEY, fnKeyMap[y][x], 0);
						}
						else
						{
							emit(uinputFd, EV_KEY, keyMap[y][x], 0);
						}
						printf("keyup\n\r");
					   	emit(uinputFd, EV_SYN, SYN_REPORT, 0);
						keyStat[x][y] = 1;
					}
				} 
			}
			row = 0;
			delay(40);
		}

	}
}

static char *
itimerspec_dump(struct itimerspec *ts)
{
    static char buf[1024];

    snprintf(buf, sizeof(buf),
            "itimer: [ interval=%lu s %lu ns, next expire=%lu s %lu ns ]",
            ts->it_interval.tv_sec,
            ts->it_interval.tv_nsec,
            ts->it_value.tv_sec,
            ts->it_value.tv_nsec
           );

    return (buf);
}
