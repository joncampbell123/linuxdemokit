
#include <sys/mman.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <jpeglib.h>

Display*			x_display = NULL;
Visual*				x_visual = NULL;
Screen*				x_screen = NULL;
int				x_depth = 0;
Atom				x_wmDelete;
XEvent				x_event;
Colormap			x_cmap;
XSetWindowAttributes		x_swa;
XWindowAttributes		x_gwa;
GC				x_gc = 0;
XImage*				x_image = NULL;
XShmSegmentInfo			x_shminfo;
size_t				x_image_length = 0;
int				x_using_shm = 0;
Window				x_root_window;
Window				x_window = (Window)0;
int				x_screen_index = 0;
int				x_quit = 0;
int				bitmap_width = 0;
int				bitmap_height = 0;

unsigned char*			image=NULL; /* 24bpp image */
size_t				image_stride;
size_t				image_width,image_height;

void close_bitmap();

int init_shm() {
	close_bitmap();

	if ((x_gc=XCreateGC(x_display, (Drawable)x_window, 0, NULL)) == NULL) {
		fprintf(stderr,"Cannot create drawable\n");
		close_bitmap();
		return 0;
	}

	x_using_shm = 1;
	memset(&x_shminfo,0,sizeof(x_shminfo));
	bitmap_width = (x_gwa.width+15)&(~15);
	bitmap_height = (x_gwa.height+15)&(~15);
	if ((x_image=XShmCreateImage(x_display, x_visual, x_depth, ZPixmap, NULL, &x_shminfo, bitmap_width, bitmap_height)) == NULL) {
		fprintf(stderr,"Cannot create SHM image\n");
		close_bitmap();
		return 0;
	}

	x_image_length = x_image->bytes_per_line * (x_image->height + 1);
	if ((x_shminfo.shmid=shmget(IPC_PRIVATE, x_image_length, IPC_CREAT | 0777)) < 0) {
		fprintf(stderr,"Cannot get SHM ID for image\n");
		x_shminfo.shmid = 0;
		close_bitmap();
		return 0;
	}

	if ((x_shminfo.shmaddr=x_image->data=(char*)shmat(x_shminfo.shmid, 0, 0)) == MAP_FAILED) {
		fprintf(stderr,"Cannot mmap for image\n");
		x_shminfo.shmaddr = NULL;
		x_image->data = NULL;
		close_bitmap();
		return 0;
	}
	x_shminfo.readOnly = 0;
	XShmAttach(x_display, &x_shminfo);
	XSync(x_display, 0);

	memset(x_image->data,0x00,x_image_length);

	bitmap_width = x_gwa.width;
	bitmap_height = x_gwa.height;
	fprintf(stderr,"SHM-based XImage created\n");
	return 1;
}

int init_norm() {
	close_bitmap();

	if ((x_gc=XCreateGC(x_display, (Drawable)x_window, 0, NULL)) == NULL) {
		fprintf(stderr,"Cannot create drawable\n");
		close_bitmap();
		return 0;
	}

	x_using_shm = 0;
	bitmap_width = (x_gwa.width+15)&(~15);
	bitmap_height = (x_gwa.height+15)&(~15);
	if ((x_image=XCreateImage(x_display, x_visual, x_depth, ZPixmap, 0, NULL, bitmap_width, bitmap_height, 32, 0)) == NULL) {
		fprintf(stderr,"Cannot create image\n");
		close_bitmap();
		return 0;
	}

	x_image_length = x_image->bytes_per_line * (x_image->height + 1);
	if ((x_image->data=(char*)mmap(NULL,x_image_length,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0)) == MAP_FAILED) {
		fprintf(stderr,"Cannot mmap for image\n");
		x_image->data = NULL;
		close_bitmap();
		return 0;
	}

	memset(x_image->data,0x00,x_image_length);

	bitmap_width = x_gwa.width;
	bitmap_height = x_gwa.height;
	fprintf(stderr,"Normal (non-SHM) XImage created\n");
	return 1;
}

void close_bitmap() {
	if (x_shminfo.shmid != 0) {
		XShmDetach(x_display, &x_shminfo);
		shmctl(x_shminfo.shmid, IPC_RMID, 0); /* the buffer will persist until X closes it */
	}
	if (x_image != NULL) {
		if (x_image) {
			if (x_image->data != NULL) {
				shmdt(x_image->data);
				x_image->data = NULL;
				x_image_length = 0;
			}
		}
		else {
			if (x_image->data != NULL) {
				munmap(x_image->data, x_image_length);
				x_image->data = NULL;
				x_image_length = 0;
			}
		}
		XDestroyImage(x_image);
		x_image = NULL;
	}
	if (x_gc != NULL) {
		XFreeGC(x_display, x_gc);
		x_gc = NULL;
	}
	x_shminfo.shmid = 0;
	x_using_shm = 0;
}

void update_to_X11() {
	if (x_image == NULL || bitmap_width == 0 || bitmap_height == 0)
		return;

	if (x_using_shm)
		XShmPutImage(x_display, x_window, x_gc, x_image, 0, 0, 0, 0, bitmap_width, bitmap_height, 0);
	else
		XPutImage(x_display, x_window, x_gc, x_image, 0, 0, 0, 0, bitmap_width, bitmap_height);
}

/* render image to XImage.
 * stretch fit using crude nearest neighbor scaling */
void rerender_out() {
	unsigned char *srow;
	unsigned char *drow;
	int ox,oy; // in pixels
	int sx,sy; // in 16.16 fixed point
	int stepx;

	stepx = (image_width << 16) / bitmap_width;
	for (oy=0;oy < bitmap_height;oy++) {
		srow = (((oy * image_height) / bitmap_height) * image_stride) + image;
		drow = x_image->data + (x_image->bytes_per_line * oy);

		if (x_image->bits_per_pixel == 32) {
			if (x_image->blue_mask == 0x000000FF) {/*most common, ARGB*/
				for (ox=0,sx=0;ox < bitmap_width;ox++,sx += stepx) {
					unsigned char *s = srow + ((sx >> 16)*3);
					*drow++ = s[2]; /* libjpeg RGB -> ARGB */
					*drow++ = s[1];
					*drow++ = s[0];
					*drow++ = 0xFF; // alpha, I assume
				}
			}
			else if (x_image->blue_mask == 0x00FF0000) {/*also common, ABGR*/
				for (ox=0,sx=0;ox < bitmap_width;ox++,sx += stepx) {
					unsigned char *s = srow + ((sx >> 16)*3);
					*drow++ = s[0]; /* libjpeg RGB -> ABGR */
					*drow++ = s[1];
					*drow++ = s[2];
					*drow++ = 0xFF; // alpha, I assume
				}
			}
			else {
				fprintf(stderr,"WARNING: unsupported ARGB channel order\n");
				fprintf(stderr,"     rmask=0x%08x gmask=0x%08x bmask=0x%08x\n",
					x_image->red_mask,
					x_image->green_mask,
					x_image->blue_mask);
				break;
			}
		}
		else {
			fprintf(stderr,"WARNING: unsupported bit depth %u/bpp\n",
				x_image->bits_per_pixel);
			break;
		}
	}
}

int main() {
	int redraw = 1;

	memset(&x_shminfo,0,sizeof(x_shminfo));

	/* WARNING: There are a LOT of steps involved at this level of X-windows Xlib interaction! */

	if ((x_display=XOpenDisplay(NULL)) == NULL) {
		if ((x_display=XOpenDisplay(":0")) == NULL) {
			fprintf(stderr,"Unable to open X display\n");
			return 1;
		}
	}

	x_root_window = DefaultRootWindow(x_display);
	x_screen_index = DefaultScreen(x_display);
	x_screen = XScreenOfDisplay(x_display,x_screen_index);
	x_depth = DefaultDepthOfScreen(x_screen);

	fprintf(stderr,"Root window: id=%lu depth=%lu screenindex=%d\n",
		(unsigned long)x_root_window,
		(unsigned long)x_depth,
		x_screen_index);

	/* we want to respond to WM_DELETE_WINDOW */
	x_wmDelete = XInternAtom(x_display,"WM_DELETE_WINDOW", True);

	if ((x_visual=DefaultVisualOfScreen(x_screen)) == NULL) {
		fprintf(stderr,"Cannot get default visual\n");
		return 1;
	}

	x_cmap = XCreateColormap(x_display, x_root_window, x_visual, AllocNone);

	x_swa.colormap = x_cmap;
	x_swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask; /* send us expose events, keypress/keyrelease events */

	x_window = XCreateWindow(x_display, x_root_window, 0, 0, 640, 480, 0, x_depth, InputOutput, x_visual, CWEventMask, &x_swa);

	XMapWindow(x_display, x_window);
	XMoveWindow(x_display, x_window, 0, 0);
	XStoreName(x_display, x_window, "Hello world");

	XSetWMProtocols(x_display, x_window, &x_wmDelete, 1); /* please send us WM_DELETE_WINDOW event */

	XGetWindowAttributes(x_display, x_window, &x_gwa);

	/* now we need an XImage. Try to use SHM (shared memory) mapping if possible, else, don't */
	if (!init_shm()) {
		if (!init_norm()) {
			fprintf(stderr,"Cannot alloc bitmap\n");
			return 1;
		}
	}

	/* decode a JPEG image for use in this program */
	{
		struct jpeg_decompress_struct decom;
		struct jpeg_error_mgr errmgr;
		FILE *fp;

		if ((fp=fopen("../../res/kitten1280x800.jpg","rb")) == NULL) {
			fprintf(stderr,"Failed to open kitten1280x800.jpg, %s\n",strerror(errno));
			return 1;
		}
		decom.err = jpeg_std_error(&errmgr);
		jpeg_create_decompress(&decom);
		jpeg_stdio_src(&decom,fp);
		jpeg_read_header(&decom,1);
		if (decom.image_width == 0 || decom.image_height == 0) {
			fprintf(stderr,"JPEG image has no dimensions\n");
			return 1;
		}

		image_width = decom.image_width;
		image_height = decom.image_height;
		image_stride = ((image_width+15)&(~15)) * 3;
		image = malloc(image_stride * image_height);

		/* convert to RGB please */
		decom.out_color_space = JCS_RGB;
		jpeg_start_decompress(&decom);

		{
			JSAMPARRAY rows = (JSAMPROW*)malloc(sizeof(JSAMPROW) * decom.image_height);
			int y;

			for (y=0;y < decom.image_height;y++)
				rows[y] = (char*)image + (y * image_stride);

			while (decom.output_scanline < decom.image_height)
				jpeg_read_scanlines(&decom,rows+decom.output_scanline,decom.image_height-decom.output_scanline);

			free(rows);
		}

		jpeg_finish_decompress(&decom);
		jpeg_destroy_decompress(&decom);
		fclose(fp);
	}

	rerender_out();

	/* wait for events */
	while (!x_quit) {
		/* you can skip XSync and go straight to XPending() if you are doing animation */
		if (XPending(x_display)) {
			XNextEvent(x_display, &x_event);

			if (x_event.type == Expose) {
				fprintf(stderr,"Expose event\n");
				redraw = 1;

				/* this also seems to be a good way to detect window resize */
				{
					int pw = x_gwa.width;
					int ph = x_gwa.height;

					XGetWindowAttributes(x_display, x_window, &x_gwa);

					if (pw != x_gwa.width || ph != x_gwa.height) {
						close_bitmap();
						if (!init_shm()) {
							if (!init_norm()) {
								fprintf(stderr,"Cannot alloc bitmap\n");
								return 1;
							}
						}

						rerender_out();
					}
				}
			}
			else if (x_event.type == KeyPress) {
				char buffer[80];
				KeySym sym=0;

				XLookupString(&x_event.xkey, buffer, sizeof(buffer), &sym, NULL);

				if (sym == XK_Escape) {
					fprintf(stderr,"Exit, by ESC\n");
					x_quit = 1;
				}
			}
			else if (x_event.type == ClientMessage) {
				if ((Atom)x_event.xclient.data.l[0] == x_wmDelete) {
					fprintf(stderr,"Exit, by window close\n");
					x_quit = 1;
				}
			}
		}

		if (redraw) {
			update_to_X11();
			redraw = 0;
		}

		XSync(x_display,False);
	}

	/* also a lot involved for cleanup */
	close_bitmap();
	free(image);
	XDestroyWindow(x_display,x_window);
	XCloseDisplay(x_display);
	return 0;
}

