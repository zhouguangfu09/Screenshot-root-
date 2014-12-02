#ifndef FBBUFFER_H
#define FBBUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <byteswap.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mntent.h>
#include <errno.h>
#include <sys/utsname.h>

#include <sys/vt.h>
#include <linux/fb.h>

#include "log.h"

#define DEFAULT_FB      "/dev/graphics/fb0"
#define RED565(x)    ((((x) >> (11 )) & 0x1f) << 3)
#define GREEN565(x)  ((((x) >> (5 )) & 0x3f) << 2)
#define BLUE565(x)   ((((x) >> (0)) & 0x1f) << 3)

#define ALPHA1555(x) ((((x) >> (15)) & 0x1 ) << 0)
#define RED1555(x)   ((((x) >> (10)) & 0x1f) << 3)
#define GREEN1555(x) ((((x) >> (5 )) & 0x1f) << 3)
#define BLUE1555(x)  ((((x) >> (0 )) & 0x1f) << 3)

struct picture{
  int xres,yres;
  unsigned char* buffer;
  struct fb_cmap *colormap;
  char bps,gray;
};

static char optstring[] = "hiC:c:d:s:";
static struct option long_options[] = {
        {"slowcon", 1, 0, 'C'},
        {"console", 1, 0, 'c'},
        {"device", 1, 0, 'd'},
        {"help", 0, 0, 'h'},
        {"noint", 0, 0, 'i'},
        {"sleep", 1, 0, 's'},
        {0, 0, 0, 0}
        };

#if 0
static int Write_RAW(struct picture* pict, char* filename)
{
  int fd = open(filename, O_CREAT | O_WRONLY | O_SYNC, 0777);
  if (fd < 0)	return -1;

  // convert picture to 32-bit format
  printf ("BPS: %d\n", pict->bps);
  switch (pict->bps)
  {
  	case 8:
		convert8to32 (pict);
		break;
	case 15:
		convert1555to32 (pict);
		break;
	case 16:
		convert565to32 (pict);
		break;
	case 32:
		break;
	default:
		return -1;
  }

  Write (fd, pict->buffer, pict->xres*pict->yres*4);
  close (fd);

  return 0;
}
#endif

void FatalError(char* err);

void Usage(char *binary);

void Help(char *binary);

void chvt(int num);

unsigned int create_bitmask(struct fb_bitfield* bf);

// Unifies the picture's pixel format to be 32-bit ARGB
void unify(struct picture *pict, struct fb_var_screeninfo *fb_varinfo);

int read_fb(char *device, struct picture *pict);


void convert8to32(struct picture *pict);

void convert1555to32(struct picture *pict);

void convert565to32(struct picture *pict);

int TakeScreenshot (char* device, struct picture* pict);
unsigned char* read_fb_buffer();
void freefbb(unsigned char* fb);

#endif
