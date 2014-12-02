#include "fbbuffer.h"

static int waitbfg=0; /* wait before grabbing (for -C )... */


/* some conversion macros */
#define RED565(x)    ((((x) >> (11 )) & 0x1f) << 3)
#define GREEN565(x)  ((((x) >> (5 )) & 0x3f) << 2)
#define BLUE565(x)   ((((x) >> (0)) & 0x1f) << 3)

#define ALPHA1555(x) ((((x) >> (15)) & 0x1 ) << 0)
#define RED1555(x)   ((((x) >> (10)) & 0x1f) << 3)
#define GREEN1555(x) ((((x) >> (5 )) & 0x1f) << 3)
#define BLUE1555(x)  ((((x) >> (0 )) & 0x1f) << 3)

void FatalError(char* err){
	fprintf(stderr,"An error occured: %s %s\nExiting now...\n",err,strerror(errno));
	fflush(stderr);
	exit (1);
}

void Usage(char *binary){
	printf("Usage: %s [-ghi] [-{C|c} vt] [-d dev] [-s n] filename.png\n", binary);
}

void Help(char *binary){
	printf("\t\tby Dariusz Swiderski <sfires@sfires.net>\n\n");

	Usage(binary);

	printf("\nPossible options:\n");
	printf("\t-C n  \tgrab from console n, for slower framebuffers\n");
	printf("\t-c n  \tgrab from console n\n");
	printf("\t-d dev\tuse framebuffer device dev instead of default\n");
	/* not supported as for now
    printf("\t-g    \tsave a grayscaled PNG\n");
	 */
	printf("\t-h    \tprint this usage information\n");
	printf("\t-i    \tturns OFF interlacing\n");
	printf("\t-s n  \tsleep n seconds before making screenshot\n");

	printf("\nSend feedback !!!\n");
}

void chvt(int num){
	int fd;
	if(!(fd = open("/dev/console", O_RDWR)))
		FatalError("cannot open /dev/console");
	if (ioctl(fd, VT_ACTIVATE, num))
		FatalError("ioctl VT_ACTIVATE ");
	if (ioctl(fd, VT_WAITACTIVE, num))
		FatalError("ioctl VT_WAITACTIVE");
	close(fd);
	if (waitbfg)
		sleep (3);
}

unsigned int create_bitmask(struct fb_bitfield* bf) {

	return ~(~0u << bf->length) << bf->offset;
}

// Unifies the picture's pixel format to be 32-bit ARGB
void unify(struct picture *pict, struct fb_var_screeninfo *fb_varinfo) {

	__u32 red_mask, green_mask, blue_mask;
	__u32 c;
	__u32 r, g, b;
	__u32* out;
	int i, j = 0, bytes_pp;

	// build masks for extracting colour bits
	red_mask = create_bitmask(&fb_varinfo->red);
	green_mask = create_bitmask(&fb_varinfo->green);
	blue_mask = create_bitmask(&fb_varinfo->blue);

	// go through the image and put the bits in place
	out = (__u32*)malloc(pict->xres * pict->yres * sizeof(__u32));
	bytes_pp = pict->bps >> 3;
	for (i = 0; i < pict->xres * pict->yres * bytes_pp; i += bytes_pp) {

		memcpy (((char*)&c) + (sizeof(__u32) - bytes_pp), pict->buffer + i, bytes_pp);

		// get the colors
		r = ((c & red_mask) >> fb_varinfo->red.offset) & ~(~0u << fb_varinfo->red.length);
		g = ((c & green_mask) >> fb_varinfo->green.offset) & ~(~0u << fb_varinfo->green.length);
		b = ((c & blue_mask) >> fb_varinfo->blue.offset) & ~(~0u << fb_varinfo->blue.length);

		// format the new pixel
		out[j++] = (0xFF << 24) | (b << 16) | (g << 8) | r;
	}

	free(pict->buffer);
	pict->buffer = (char*)out;
	pict->bps = 32;
}


int read_fb(char *device, struct picture *pict)
{
	int fd, i,j;
	struct fb_fix_screeninfo fb_fixinfo;
	struct fb_var_screeninfo fb_varinfo;

	if(!(fd=open(device, O_RDONLY)))
		D("Couldn't open framebuffer device");

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fb_fixinfo))
		D("ioctl FBIOGET_FSCREENINFO");

	if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_varinfo))
		D("ioctl FBIOGET_VSCREENINFO");

	pict->xres=fb_varinfo.xres;
	pict->yres=fb_varinfo.yres;
	pict->bps=fb_varinfo.bits_per_pixel;
	pict->gray=fb_varinfo.grayscale;

	if(fb_fixinfo.visual==FB_VISUAL_PSEUDOCOLOR){
		pict->colormap=(struct fb_cmap*)malloc(sizeof(struct fb_cmap));
		pict->colormap->red=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
		pict->colormap->green=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
		pict->colormap->blue=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
		pict->colormap->transp=(__u16*)malloc(sizeof(__u16)*(1<<pict->bps));
		pict->colormap->start=0;
		pict->colormap->len=1<<pict->bps;
		if (ioctl(fd, FBIOGETCMAP, pict->colormap))
			D("ioctl FBIOGETCMAP");
	}

	switch(pict->bps){
	case 15:
		i=2;
		break;
	default:
		i=pict->bps>>3;
	}

	if(!(pict->buffer=malloc(pict->xres*pict->yres*i)))
		D("couldnt malloc.");

	fflush(stdout);

	if(read(fd, pict->buffer, ((pict->xres * pict->yres) * i) )!=(pict->xres * pict->yres *i ))
		D("couldn't read fb0.");

	close (fd);

	unify(pict, &fb_varinfo);
	return 0;
}


void convert8to32(struct picture *pict){
	int i;
	int j=0;
	__u8 c;
	char *out=(char*)malloc(pict->xres*pict->yres*4);
	for (i=0; i<pict->xres*pict->yres; i++)
	{
		c = ((__u8*)(pict->buffer))[i];
		out[j++]=(char)(pict->colormap->red[c]);
		out[j++]=(char)(pict->colormap->green[c]);
		out[j++]=(char)(pict->colormap->blue[c]);
		out[j++]=(char)(pict->colormap->transp[c]);
	}
	free(pict->buffer);
	pict->buffer=out;
}

void convert1555to32(struct picture *pict){
	int i;
	int j=0;
	__u16 t,c;
	char *out=(char*)malloc(pict->xres*pict->yres*4);
	for (i=0; i<pict->xres*pict->yres; i++)
	{
		c = ( (__u16*)(pict->buffer))[i];
		out[j++]=(char)RED1555(c);
		out[j++]=(char)GREEN1555(c);
		out[j++]=(char)BLUE1555(c);
		out[j++]=(char)ALPHA1555(c);
	}
	free(pict->buffer);
	pict->buffer=out;
}

void convert565to32(struct picture *pict){
	int i;
	int j=0;
	__u16 t,c;
	char *out=(char*)malloc(pict->xres*pict->yres*4);
	for (i=0; i<pict->xres*pict->yres; i++)
	{
		c = ( (__u16*)(pict->buffer))[i];
		out[j++]=(char)0xff;
		out[j++]=(char)RED565(c);
		out[j++]=(char)GREEN565(c);
		out[j++]=(char)BLUE565(c);

	}
	free(pict->buffer);
	pict->buffer=out;
}

int TakeScreenshot (char* device, struct picture* pict)
{
	return read_fb(device, pict);
}

unsigned char* read_fb_buffer()
{
	D("read fb0");
	char* device;
	device ="/dev/graphics/fb0";
	struct picture pict, temp_pict;

	if (TakeScreenshot(device, &pict) < 0)
		D("Take screen failed.");

	D("read fb0 successfully");
	int length = pict.xres * pict.yres * pict.bps / 8;
	int	seek, len;
	int temp_seak,temp_len, index = 0;
	int temp_length = length/4;
	temp_pict.xres = pict.xres/2;
	temp_pict.yres = pict.yres/2;
	int linebytes = pict.xres*pict.bps/8;
	temp_pict.buffer = malloc(temp_length*sizeof(char));
	short nextline = 0;

	for(temp_seak=0;temp_seak<(temp_length-4);temp_seak += 4){
		temp_pict.buffer[temp_seak] = (pict.buffer[index]+pict.buffer[index+4])/2;
		temp_pict.buffer[temp_seak+1] = (pict.buffer[index+1]+pict.buffer[index+5])/2;
		temp_pict.buffer[temp_seak+2] = (pict.buffer[index+2]+pict.buffer[index+6])/2;
		temp_pict.buffer[temp_seak+3] = (pict.buffer[index+3]+pict.buffer[index+7])/2;

		if(((index+8)%(linebytes) == 0)&&nextline==1)
		{
			index += linebytes+8;
			nextline = 0;
		}
		else
		{
			index += 8;
			nextline = 1;
		}
	}
	free(pict.buffer);

    D("Size %d",temp_length);
	return temp_pict.buffer;
//    return pict.buffer;
}

void freefbb(unsigned char* fb)
{
	free(fb);
}



