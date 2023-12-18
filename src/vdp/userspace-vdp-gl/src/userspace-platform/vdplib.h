#ifndef __VDULIB_INCLUDED__
#define __VDULIB_INCLUDED__

typedef enum {
	VDU_CRSR_LEFT = 8,
	VDU_CRSR_RIGHT = 9,
	VDU_CRSR_DOWN = 10,
	VDU_CRSR_UP = 11,
	VDU_CLS = 12,
	VDU_CR = 13,
	VDU_PAGE_MODE_ON = 14,
	VDU_PAGE_MODE_OFF = 15,
	VDU_CLG = 16,
	VDU_COLOUR = 17,
	VDU_GCOL = 18,
	VDU_PALETTE = 19,
	VDU_MODE = 22,
	VDU_SYS = 23,
	VDU_GVIEWPORT = 23,
	VDU_PLOT = 25,
	VDU_RESET = 26,
	VDU_TVIEWPORT = 28,
	VDU_GRAPH_ORIG = 29,
	VDU_HOME_CRSR = 30,
	VDU_TAB = 31,
	VDU_BACKSPACE = 127,
} t_vdu_command;

#define VDU_DISABLE_CRSR(enable)         { VDU_SYS, 1, enable ? 1 : 0 }
#define VDU_INIT                         { VDU_SYS, 0, 0x80 }
#define VDU_SETPALETTE_RGB(idx, r, g, b) { VDU_PALETTE, idx, 255, r, g, b }
#define VDU_SETMODE(a)                   { VDU_MODE, a }

extern int vdplib_send(int portfd, uint8_t b);
extern int vdplib_startup(void);

#endif
