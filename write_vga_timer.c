#define COL8_000000 0
#define COL8_FF0000 1
#define COL8_00FF00 2 
#define COL8_FFFF00 3
#define COL8_0000FF 4
#define COL8_FF00FF 5
#define COL8_00FFFF 6
#define COL8_FFFFFF 7
#define COL8_C6C6C6 8
#define COL8_840000 9
#define COL8_008400 10
#define COL8_848400 11
#define COL8_000084 12
#define COL8_840084 13
#define COL8_008484 14
#define COL8_848484 15

#define PORT_KEYDAT 0x60   //keyboard/mouse port
#define PIC1_OCW2   0xA0
#define PIC0_OCW2    0x20

#define PORT_KEYSTA 0x64   //mouse port
#define PORT_KEYCMD 0x64
#define KEYSTA_SENT_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47
#include "mem_util.h"
#include "win_sheet.h"
#include "timer.h"
#include "global_define.h"
//
extern char systemFont[16];

void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_in8(int port);

void io_sti(void);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void show_char(void);

void init_palette(void);
void set_palette(int start, int end,
		unsigned char* rgb);
void boxfill8(unsigned char * vram, int xsize, unsigned char c, int x, int y, int x0, int y0);


struct BOOTINFO {
	char* vgaRam;
	short screenX, screenY;
};
void initBootInfo(struct BOOTINFO *pBootInfo);

void showChar8(char *vram, int xsize, int x, int y, char c, char* font);

//void showString(char *vram, int xsize, int x, int y, char c, unsigned char*s);
void showString(struct SHTCTL *shtctl, struct SHEET *sht, int x, int y, char color, unsigned char* s);

void putblock(char* vram, int vxsize, int pxsize, int pysize, int px0, int py0,
		char* buf, int bxsize);
void init_mouse_cursor(char* mouse, char bc);
void intHandlerFromC(char* esp);

//static char mcursor[256];
static struct BOOTINFO bootInfo;
static char keyval[5] = {'0', 'x', 0, 0, 0};

char charToHexVal(char c);
char* charToHexStr(unsigned char c);

void wait_KBC_sendReady(void);
void init_keyboard(void);
void intHandlerFromMouse(char* esp);

//Let the key buffer hold key data
//and let the main loop run graphic operations
struct KEYBUF{
	unsigned char key_buf[32];
	int next_r, next_w, len;
};

//static struct KEYBUF keybuf;

//A queue (FIFO) for the mouse/keyboard handlers

static struct FIFO8 mouseinfo;
static struct FIFO8 keyinfo;

static char mousebuf[128];
static char keybuf[32];

struct MOUSE_DESC{
	unsigned char buf[3], phase;
	int x, y, btn;
};

static struct MOUSE_DESC mdec;

void enable_mouse(struct MOUSE_DESC* mdec);
int mouse_decode(struct MOUSE_DESC *mdec, unsigned char dat);
//void computeMousePosition(struct MOUSE_DESC *mdec);
void computeMousePosition(struct SHTCTL * shtctl, struct SHEET* sht,
struct SHEET* sht_mouse,	       
		struct MOUSE_DESC *mdec);



//void eraseMouse(char* vram);
//void drawMouse(char* vram);
//void show_mouse_info(void);
//use the mouse for handlers, write information on the background sheet
void show_mouse_info(struct SHTCTL *shtctl,struct SHEET *sht_back, struct SHEET *sht_mouse);


//Mouse simultaneous position
static int mx = 0, my = 0;

char* intToHexStr(unsigned int data);

struct AddrRangeDesc{
	unsigned int baseAddrLow;
	unsigned int baseAddrHigh;
	unsigned int lengthLow;
	unsigned int lengthHigh;
	unsigned int type;
};

char* get_adr_buffer(void);

void showMemoryInfo(struct SHTCTL* shtctl, struct SHEET* sht, struct AddrRangeDesc* desc, char* vram, int page, 
		int xsize, int color);
int get_memory_block_count();


/*
struct FREEINFO {
	unsigned int addr, size;
};

struct MEMMAN{
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
*/

struct MEMMAN* memman = (struct MEMMAN*) 0x0100000;

//void memman_init(struct MEMMAN* man);
//

static unsigned char* buf_back;
static unsigned char buf_mouse[256];
#define COLOR_INVISIBLE 99

void init_screen8(char* p, int xsize, int ysize);	

static int xsize = 0, ysize = 0;

struct SHEET* message_box(struct SHTCTL* shtctl, char *title);
void make_window8(struct SHTCTL *shtctl, struct SHEET *sht, char *title);

static struct FIFO8 timerinfo;
static char timerbuf[8];

static struct TIMERCTL timerctl;

void intHandlerFromTimer(char* esp);


void CMain(void){
	initBootInfo(&bootInfo);
	char* p = bootInfo.vgaRam;
	xsize = bootInfo.screenX;
	ysize = bootInfo.screenY; char* vram = bootInfo.vgaRam; 
	struct SHTCTL *shtctl;
	struct SHEET *sht_back = 0, *sht_mouse = 0;

	init_pit(&timerctl);
	fifo8_init(&timerinfo, 8, timerbuf);
	setTimer(&timerctl, 500, &timerinfo, 1);

	//Init the buffer to store interrupt data
	fifo8_init(&mouseinfo, 128, mousebuf);
	fifo8_init(&keyinfo, 32, keybuf);
	
	//Initialize the timer adn its buffers
//	init_pit();
//	fifo8_init(&timerinfo, 8, timerbuf);
//	setTimer(500, &timerinfo, 1);

	//char *p = (char *) 0xa0000; //Start address for vram
	//int xsize = 320, ysize = 200;
	
	init_palette();
	init_keyboard();

	//Show some fonts
	
	/*
	showChar8(p, xsize, 8, 8, COL8_FFFFFF, systemFont + 'A'*16);		
	showChar8(p, xsize, 2*8, 8, COL8_FFFFFF, systemFont + 'B'*16);
	showChar8(p, xsize, 3*8, 8, COL8_FFFFFF, systemFont + 'C'*16);
	showChar8(p, xsize, 5*8, 8, COL8_FFFFFF, systemFont + '3'*16);
	showChar8(p, xsize, 6*8, 8, COL8_FFFFFF, systemFont + '2'*16);

	showString(p, xsize, 8, 64, COL8_FFFFFF, "text!");
*/	

	
	int memCnt = get_memory_block_count();
	//memCnt = get_seg_code();
//	char* temp = intToHexStr(memCnt);
	//temp = intToHexStr(0xffffffff);
	//showString(p, xsize, 6*8, 5*8, COL8_FFFFFF, 
//			temp);

			

	/*
	mx = (xsize - 16)/2;
	mx = (ysize - 16 - 28)/2;
	init_mouse_cursor(mcursor, COL8_008484);	
	putblock(p, xsize, 16, 16, mx, my, mcursor, 16);

	*/


	//Set-up the keyboard, mouse interrupts
//	init_keyboard();	
//	enable_mouse();

	//Set-up the memory structure (2096 different blocks)
	//Each block have an address and a given size
	struct AddrRangeDesc* memDesc = (struct AddrRangeDesc*) get_adr_buffer();
	
	//Initialize blocks to zero
	memman_init(memman);
	
	memman_free(memman, 0x100000
			+ 0x8000,
			0x1FDE0000 - 0x8000);
				

	//Set-up window sheet system for the GUI
	
	shtctl = shtctl_init(memman, p, xsize, ysize);
	
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	
	buf_back = (unsigned char*) memman_alloc_4k(memman, xsize*ysize);
	
	
	sheet_setbuf(sht_back, buf_back, xsize, ysize, COLOR_INVISIBLE);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, COLOR_INVISIBLE);

	//Fill buffers
	init_screen8(buf_back, xsize, ysize);	
	init_mouse_cursor(buf_mouse, COLOR_INVISIBLE);
	
	sheet_slide(shtctl, sht_back, 0, 0);
 
	mx = (xsize - 16)/2;
	my = (ysize - 16 - 28)/2;
	sheet_slide(shtctl, sht_mouse, mx, my);


	struct SHEET* shtMsgBox = message_box(shtctl, "winddown");
	sheet_updown(shtctl, sht_back, 0);
	sheet_updown(shtctl, sht_mouse, 100);	
	

	/*
	int memTotal = memman_total(memman) / (1024*1024);
	char* pMemTotal = intToHexStr(memTotal);
	showString(p, xsize, 0, 0, COL8_FFFFFF, "total mem is: ");
	showString(p, xsize, 17*8, 0, COL8_FFFFFF, pMemTotal);
	showString(p, xsize, 28*8, 0, COL8_FFFFFF, " MB");	
*/	

	//DEBUGGING
	//
	//
	//
/*
char *temp = intToHexStr((unsigned int) shtctl);
showString(p, xsize, 13*8, 13*8, COL8_FFFFFF, temp);
temp = intToHexStr((unsigned int) buf_back);
showString(p, xsize, 10*8, 10*8, COL8_FFFFFF, temp);
*/

	

	io_sti(); //Recover the interrupt service
	enable_mouse(&mdec);

	int data = 0;
	int count = 0;
	int counter = 0;

	//struct TIMERCTL *timerctl = getTimerController();

	for(;;){	

		//Create a window with moving timer text
		/* this is for for-loop counter timing, not real-sec time
		 */
		char *pStr = intToHexStr(timerctl.count);
		counter++;	
		boxfill8(shtMsgBox->buf, 160, COL8_C6C6C6, 40, 28, 119, 43);
		showString(shtctl, shtMsgBox, 40, 28, COL8_000000, pStr);	
		
	//	char *pStr = intToHexStr(timerctl->timeout);
	//	boxfill8(shtMsgBox->buf, 160, COL8_C6C6C6, 40, 28, 119, 43);
	//	showString(shtctl, shtMsgBox, 40, 28, COL8_000000, pStr);

		io_cli();
	
		if(fifo8_status(&keyinfo) + fifo8_status(&mouseinfo)
   + fifo8_status(&timerinfo)	== 0){
		io_sti();
	//	io_hlt();
		}
		else if(fifo8_status(&keyinfo) != 0){
			io_sti();
			data = fifo8_get(&keyinfo);

	
			if (data == 0x1c){
				showMemoryInfo(shtctl, sht_back, memDesc + count, buf_back, count, xsize, COL8_FFFFFF);
				count++; 
				//Loop back
				if(count > memCnt){
					count = 0;
				}
			
				//sheet_refresh(shtctl);
				//sheet_refresh(shtctl, sht_mouse, begin, y, x, y + 16); 
			}

		}
		else if(fifo8_status(&mouseinfo) != 0){
		//	show_mouse_info();
		//show_mouse_info(struct SHTCTL *shtctl, struct SHEET *sht_mouse);
		show_mouse_info(shtctl,sht_back, sht_mouse);
		}
		else if(fifo8_status(&timerinfo) != 0){
			io_sti();
			showString(shtctl, sht_back, 0, 0, COL8_FFFFFF, "5[sec]");
		}
	
	}
		

}

void initBootInfo(struct BOOTINFO *pBootInfo){
	pBootInfo->vgaRam = (char*) 0xa0000;
	pBootInfo->screenX = 320;
	pBootInfo->screenY = 200;
}

void init_palette(void){
static  unsigned char table_rgb[16 *3] = {
        0x00,  0x00,  0x00,
        0xff,  0x00,  0x00,
        0x00,  0xff,  0x00,
        0xff,  0xff,  0x00,
        0x00,  0x00,  0xff,
        0xff,  0x00,  0xff,
        0x00,  0xff,  0xff,
        0xff,  0xff,  0xff,
        0xc6,  0xc6,  0xc6,
        0x84,  0x00,  0x00,
        0x00,  0x84,  0x00,
        0x84,  0x84,  0x00,
        0x00,  0x00,  0x84,
        0x84,  0x00,  0x84,
        0x00,  0x84,  0x84,
        0x84,  0x84,  0x84,
    };


	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char* rgb){
	int i, eflags;
	eflags = io_load_eflags();

	io_cli();

	io_out8(0x03c8, start); //set palette number
	for(i = start; i <= end; i++){
		io_out8(0x03c9, rgb[0] >> 2);
		io_out8(0x03c9, rgb[1] >> 2);
		io_out8(0x03c9, rgb[2] >> 2);

		rgb+=3;

	}

	io_store_eflags(eflags);
	return;
}

void boxfill8(unsigned char* vram, int xsize, unsigned char c,
		int x0, int y0, int x1, int y1){
	int x, y;
	for(y = y0; y <= y1; y++){
		for(x = x0; x <= x1; x++){
			vram[y*xsize + x] = c;
		}	
	}	


}

void showString(struct SHTCTL *shtctl, struct SHEET *sht, int x, int y, char color, unsigned char* s){	
	int begin = x;
	while(*s != 0x00){
		
		showChar8(sht->buf, sht->bxsize, x, y, color, systemFont + (*s)*16);
		s++;
		x+=8;

	}

	sheet_refresh(shtctl, sht, begin, y, x, y + 16); //16 because height of a font text
}

/*
void showString(char *vram, int xsize, int x, int y, char c, unsigned char*s){
	
	while(*s != 0x00){
		
		showChar8(vram, xsize, x, y, c, systemFont + (*s)*16);
		s++;
		x+=8;

	}


}
*/

void showChar8(char *vram, int xsize, int x, int y, char c, char* font){
	int i;
	char d;

	//write line by line
	for(i = 0; i < 16; i++){
		d = *(font + i);	
		//d = font[i];
		if((d & 0x80) != 0) {vram[(y+i)*xsize + x +  0] = c; }	
		if((d & 0x40) != 0) {vram[(y+i)*xsize + x + 1] = c; }	
		if((d & 0x20) != 0) {vram[(y+i)*xsize + x + 2] = c; }	
		if((d & 0x10) != 0) {vram[(y+i)*xsize + x +  3] = c; }	
		if((d & 0x08) != 0) {vram[(y+i)*xsize + x + 4] = c; }	
		if((d & 0x04) != 0) {vram[(y+i)*xsize + x + 5] = c; }	
		if((d & 0x02) != 0) {vram[(y+i)*xsize + x + 6] = c; }	
		if((d & 0x01) != 0) {vram[(y+i)*xsize + x + 7] = c; }	

	}	

}

void init_mouse_cursor(char* mouse, char bc){
	static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
	};

	int x, y;
	for(y = 0; y < 16; y++){
		for(x = 0; x < 16; x++){
			if(cursor[y][x] == '*'){
				mouse[y*16 + x] = COL8_000000;
			}
			else if(cursor[y][x] == 'O'){
				mouse[y*16+x] = COL8_FFFFFF;
			}
			else if(cursor[y][x] == '.'){
				mouse[y*16 + x] = bc;
			}
		}
	}

}

//vram = video ram pointer
//vxsize = size of row of video memory space
//pxsize, pysize (size of sprite/data)
//px0, py0 (upper left corner of sprite)
//buf (sprite data)
//bxsize = size of row of sprite data space
//GOAL: move the buffer into the appropriate space in the vram
void putblock(char* vram, int vxsize, int pxsize, int pysize, int px0, int py0,
		char* buf, int bxsize){

int i, j;
	for(j = 0; j < pysize; j++){
	for(i = 0; i < pxsize; i++){
			vram[(py0 + j)*vxsize + (px0 + i) ] 
				= buf[(j)*bxsize + (i)];	
	}
	}
}


void intHandlerFromC(char* esp){
	char* vram = bootInfo.vgaRam;
	int xsize = bootInfo.screenX, ysize = bootInfo.screenY;


	//io_out8(PIC0_OCW2, 0x21);
	io_out8(PIC0_OCW2,0x20);

	unsigned char data = 0;
	data = io_in8(PORT_KEYDAT);

	/*
	char* pStr = charToHexStr(data);
	static int showPos = 0;
	showString(vram, xsize, 0, showPos, COL8_FFFFFF, pStr);
	showPos += 32; //0x?? takes up 32 pixels, 8 pixels of space
	showPos = showPos % ysize;
	*/
	fifo8_put(&keyinfo, data);

	/*
	if(keybuf.len< 32){
		keybuf.key_buf[keybuf.next_w] = data;
		keybuf.len++;
		keybuf.next_w = (keybuf.next_w + 1) & 0x1F;
	}
	*/
}


char charToHexVal(char c){
	if(c >= 10){
		return	'A' + c - 10;
	}
	return '0' + c;
}

//for data interrupter private usage
char* charToHexStr(unsigned char data){
	char mod = data % 16;	
	keyval[3] = charToHexVal(mod);
	data = (data >> 4);
	keyval[2] = charToHexVal(data);

	return keyval;
}

//convert 32 bits (int) of hex data into a printable string
char* intToHexStr(unsigned int data){
	static char str[11];
	str[0] = '0';
	str[1] = 'X';
	str[10] = 0;

	//Loop four bits at a time
	int realIndex = 2;
	for(int i = 7; i >= 0; i--){
		int e = data & 0x0F;	
		if(e < 10){
			str[realIndex + i] = '0' + e;
		}
		else{
			str[realIndex + i] = 'A' + e - 10;
		}
		data = (data >> 4);	
	}	

	return str;
}



//Loop until the mouse port is free to use
//the second LSB signifies if the port is ready or not
void wait_KBC_sendReady(void){
	for(;;){
		if((io_in8(PORT_KEYSTA) & KEYSTA_SENT_NOTREADY) == 0){
			break;
		}	
	}
	return;
}

//Prepare the keyboard for interrupt-sharing with the mouse
void init_keyboard(void){
	wait_KBC_sendReady();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendReady();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4
void enable_mouse(struct MOUSE_DESC* mdec){
	wait_KBC_sendReady();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendReady();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0;
	return;
}

void intHandlerFromMouse(char* esp){

	unsigned char data;
	io_out8(PIC1_OCW2, 0x20);
	io_out8(PIC0_OCW2, 0x20);
	
	data = io_in8(PORT_KEYDAT);
	
	fifo8_put(&mouseinfo, data);
	//char* vram = bootInfo.vgaRam;
	//int xsize = bootInfo.screenX, ysize = bootInfo.screenY;

	/*
	showString(vram, xsize, 0, 0, COL8_FFFFFF, "PS/2 Mouse Handler");
	for(;;){
		io_hlt();
	}
	*/
	return;

}

//Decoder for mouse interrupt signals
int mouse_decode(struct MOUSE_DESC *mdec, unsigned char data){
	if (mdec->phase == 0){
		//start code for mouse is 0xfa
		if(data == 0xfa){
			mdec->phase = 1;
		}
		return 0;
	}

	if (mdec->phase == 1){
		//check if first data is valid
		if((data & 0xc8) == 0x08){
			mdec->buf[0] = data;
			mdec->phase = 2;
		}	
		return 0;
	}

	if (mdec->phase == 2){
		//continue storing mouse data
		mdec->buf[1] = data;
		mdec->phase = 3;
		return 0;
	}

	else if (mdec->phase == 3){
		mdec->buf[2] = data;
		mdec->phase = 1;

		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x   = mdec->buf[1];
		mdec->y   = mdec->buf[2];
		if((mdec->buf[0] & 0x10) != 0){
			mdec->x |= 0xffffff00;
		}
		if((mdec->buf[0] & 0x20) != 0){
			mdec->y |= 0xffffff00;
		}
		mdec->y = -mdec->y;
		return 1;
	}
	return -1;

}

void computeMousePosition(struct SHTCTL * shtctl, struct SHEET* sht,
	       struct SHEET* sht_mouse,
       	       struct MOUSE_DESC *mdec){

	int xsize = bootInfo.screenX, ysize = bootInfo.screenY;

	sht_mouse->bxsize = 16;
	sht_mouse->bysize = 16;

	mx += mdec->x;
	my += mdec->y;

	if(mx < 0){
		mx = 0;
	}

	if (my < 0){
		my = 0;
	}	

	
	if (mx > xsize - 16){
		//will erase from teh other side
		//sht_mouse->bxsize = xsize - mx;
	}

	if (my > ysize - 16){
		sht_mouse->bysize = ysize - my;
	}
	
	int edge = 1;
	if (mx > xsize - edge){
		mx = xsize - edge;
		sht_mouse->bxsize = edge;
	}
	if (my > ysize - edge){
		my = ysize - edge;
		sht_mouse->bysize = edge;
	}

	showString(shtctl, sht, 0, 0, COL8_FFFFFF, "improve string showing!");
}

/*
void eraseMouse(char* vram){
	int xsize = bootInfo.screenX, ysize = bootInfo.screenY;

	boxfill8(vram, xsize, COL8_008484, mx, my, mx+15, my+15);
}

void drawMouse(char* vram){
	int xsize = bootInfo.screenX, ysize = bootInfo.screenY;

	putblock(vram, xsize, 16, 16, mx, my, mcursor, 16);	
}
*/
/*
void show_mouse_info(void){
	char* vram = bootInfo.vgaRam;
	unsigned char data = 0;

	//io_sti();
	data = fifo8_get(&mouseinfo);
	if(mouse_decode(&mdec, data) == 1){
		eraseMouse(vram);
		computeMousePosition(&mdec);
		drawMouse(vram);
	}

}
*/
void show_mouse_info(struct SHTCTL *shtctl, struct SHEET *sht_back, struct SHEET *sht_mouse){
	char* vram = buf_back;
	unsigned char data = 0;

	io_sti();
	data = fifo8_get(&mouseinfo);
	if (mouse_decode(&mdec, data) == 1){
	//	computeMousePosition(&mdec);
		computeMousePosition(shtctl, sht_back,sht_mouse, &mdec);
		//Recompute how much to slide by
		sheet_slide_mouse(shtctl, sht_mouse, mx, my);
		//sheet_slide(shtctl, sht_mouse, mx, my);

	}
}
	
void showMemoryInfo(struct SHTCTL *shtctl, struct SHEET * sht,
		struct AddrRangeDesc* desc, char* vram, int page, 
		int xsize, int color){

	int x = 0, y = 0, gap = 13*8, strLen = 10*8;

	//boxfill8(vram, xsize,COL8_008484, 0, 0, xsize, 100);
	init_screen8(sht->buf, xsize, ysize);
       
	showString(shtctl, sht, x, y, color, "page is: ");
	char* pPageCnt = intToHexStr(page);
	showString(shtctl, sht, gap, y, color, pPageCnt);
	y+=16;

	showString(shtctl, sht,  x, y, color, "BaseAddrL: ");
	char* pBaseAddrL = intToHexStr(desc->baseAddrLow);
	showString(shtctl, sht,  gap, y, color, pBaseAddrL);
	y += 16;

	showString(shtctl,sht,  x, y, color, "BaseAddrH: ");
	char* pBaseAddrH = intToHexStr(desc->baseAddrHigh);
	showString(shtctl, sht,  gap, y, color, pBaseAddrH);
	y += 16;

	showString(shtctl, sht,  x, y, color, "LengthLow: ");
	char* pLengthL = intToHexStr(desc->lengthLow);
	showString(shtctl, sht,  gap, y, color, pLengthL);
	y+=16;

	showString(shtctl, sht,  x, y, color, "LengthHigh: ");
	char* pLengthH = intToHexStr(desc->lengthHigh);
	showString(shtctl, sht,  gap, y, color, pLengthH);
	y += 16;

	showString(shtctl, sht,  x, y, color, "Type: ");
	char* pType = intToHexStr(desc->type);
	showString(shtctl, sht,  gap, y, color, pType);

}

/*
void memman_init(struct MEMMAN* man){
	man->frees = 0;
	man->maxfrees = 0;
	man->lostsize = 0;
	man->losts = 0;
}

*/

void init_screen8(char* p, int xsize, int ysize){
		//Paint background
		
	boxfill8(p, xsize, COL8_008484,
		       	0, 0, 
			xsize-1, ysize-29);

	//Paint horizontal line
	boxfill8(p, xsize, COL8_C6C6C6,
		       	0, ysize - 28, 
			xsize-1, ysize-28);
	boxfill8(p, xsize, COL8_FFFFFF,
			0, ysize-27,
			xsize-1, ysize-27);

	//Paint lower task bar
	boxfill8(p, xsize, COL8_C6C6C6,
			0, ysize-26,
			xsize-1, ysize-1);
	
	//Paint lower left button
	boxfill8(p, xsize, COL8_FFFFFF,
			3, ysize-24,
			59, ysize-24);
	boxfill8(p, xsize, COL8_FFFFFF,
			2, ysize-24,
			2, ysize-4);
	boxfill8(p, xsize, COL8_848484,
			3, ysize-4,
			59, ysize-4);
	boxfill8(p, xsize, COL8_848484,
			59, ysize-23,
			59, ysize-5);
	boxfill8(p, xsize, COL8_000000,
			2, ysize-3,
			59, ysize-3);
	boxfill8(p, xsize, COL8_000000,
			60, ysize-24,
			60, ysize-3);
		
	
	//paint lower right button
	boxfill8(p, xsize, COL8_FFFFFF,
			xsize-47, ysize-3,
			xsize-4, ysize-3);
	boxfill8(p, xsize, COL8_FFFFFF,
			xsize-3, ysize-24,
			xsize-3, ysize-4);
	boxfill8(p, xsize, COL8_848484,
			xsize-47, ysize-24,
			xsize-4, ysize-24);
	boxfill8(p, xsize, COL8_848484,
			xsize-47, ysize-23,
			xsize-47, ysize-4);
	

}

void make_window8(struct SHTCTL *shtctl, struct SHEET *sht, char *title){
static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@", 
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"
    };
	
    int x, y;
    char c;
    int bxsize = sht->bxsize;
    int bysize = sht->bysize;

    boxfill8(sht->buf, bxsize, COL8_C6C6C6, 0, 0, bxsize - 1, 0);
    boxfill8(sht->buf, bxsize, COL8_FFFFFF, 1, 1, bxsize - 2, 1);
    boxfill8(sht->buf, bxsize, COL8_C6C6C6, 0, 0, 0, bysize - 1);
    boxfill8(sht->buf, bxsize, COL8_C6C6C6, 1, 1, 1, bysize - 1);
    boxfill8(sht->buf, bxsize, COL8_848484, bxsize - 2, 1, bxsize - 2, bysize - 2);
    boxfill8(sht->buf, bxsize, COL8_000000, bxsize - 1, 0, bxsize - 1, bysize - 1);
    boxfill8(sht->buf, bxsize, COL8_C6C6C6, 2, 2, bxsize - 3, bysize - 3);
    boxfill8(sht->buf, bxsize, COL8_000084, 3, 3, bxsize - 4, 20);
    boxfill8(sht->buf, bxsize, COL8_848484, 1, bysize - 2, bxsize - 2, bysize - 2);
    boxfill8(sht->buf, bxsize, COL8_000000, 0, bysize - 1, bxsize - 1, bysize - 1);

    showString(shtctl, sht, 24, 4, COL8_FFFFFF, title);

    for (y = 0; y < 14; y++){

	for (x = 0; x < 16; x++){

		c = closebtn[y][x];
		if (c == '@'){
			c = COL8_000000;
		}
		else if (c == '$'){
			c = COL8_848484;
		}
		else if (c == 'Q'){
			c = COL8_C6C6C6;
		}
		else{
			c = COL8_FFFFFF;
		}

		sht->buf[(5 + y) * sht->bxsize + (sht->bxsize - 21 + x)] = c;
		
	}

    }

}


struct SHEET* message_box(struct SHTCTL* shtctl, char *title){
	struct SHEET* sht_win;
	unsigned char *buf_win;

	sht_win = sheet_alloc(shtctl);
	buf_win = (unsigned char *) memman_alloc_4k(memman, 160*68);
	sheet_setbuf(sht_win, buf_win, 160, 68, -1);

	make_window8(shtctl, sht_win, title);
	
	//Message in the message box
	showString(shtctl, sht_win, 24, 28, COL8_000000, "Welecome to");

	showString(shtctl, sht_win, 24, 44, COL8_000000, "myOS");


	sheet_slide(shtctl, sht_win, 80, 72);
	sheet_updown(shtctl, sht_win, 1);

	return sht_win;
}

void intHandlerFromTimer(char *esp){


	io_out8(0x20, 0x60);
	timerctl.count++;

	if (timerctl.timeout > 0){

		timerctl.timeout--;
		if (timerctl.timeout == 0){
			fifo8_put(timerctl.fifo, timerctl.data);
		}
	}
	return;
}


