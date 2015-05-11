
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/xcb.h>

xcb_connection_t*		xcb_connection = NULL;
xcb_screen_t*			xcb_screen = NULL;
xcb_window_t			xcb_window = NULL;
xcb_generic_event_t*		xcb_event = NULL;
const xcb_setup_t*		xcb_setup = NULL;
int				xcb_screen_num = -1;
xcb_get_geometry_reply_t*	xcb_geo = NULL;
xcb_gcontext_t			xcb_gc = 0;

xcb_pixmap_t			xcb_pixmap = 0;
int				bitmap_width = 0,bitmap_height = 0;
int				redraw = 1;

void init_bitmap() {
	if (xcb_pixmap != 0)
		return;

	if (xcb_geo == NULL)
		return;

	bitmap_width = (xcb_geo->width+15)&(~15);
	bitmap_height = (xcb_geo->height+15)&(~15);

	xcb_pixmap = xcb_generate_id(xcb_connection);
	xcb_create_pixmap(xcb_connection, xcb_screen->root_depth, xcb_pixmap, xcb_window, bitmap_width, bitmap_height);

	bitmap_width = xcb_geo->width;
	bitmap_height = xcb_geo->height;
}

void free_bitmap() {
	if (xcb_pixmap != 0) {
		xcb_free_pixmap(xcb_connection, xcb_pixmap);
		xcb_pixmap = 0;
	}
}

void update_xcb_geo() {
	xcb_get_geometry_cookie_t x;

	if (xcb_geo != NULL) {
		free(xcb_geo);
		xcb_geo = NULL;
	}

	x = xcb_get_geometry(xcb_connection, xcb_window);
	xcb_geo = xcb_get_geometry_reply(xcb_connection, x, NULL);

	if (xcb_geo) {
		fprintf(stderr,"Window is now %u x %u\n",xcb_geo->width,xcb_geo->height);
	}
}

void freeall() {
	free_bitmap();
	if (xcb_geo != NULL) {
		free(xcb_geo);
		xcb_geo = NULL;
	}
}

int main() {
	/* WARNING: a lot is involved at the XCB low level interface */

	if ((xcb_connection=xcb_connect(NULL, &xcb_screen_num)) == NULL) {
		if ((xcb_connection=xcb_connect(":0", &xcb_screen_num)) == NULL) {
			fprintf(stderr,"Unable to create xcb connection\n");
			return 1;
		}
	}

	if ((xcb_setup=xcb_get_setup(xcb_connection)) == NULL) { /* "the result must not be freed" */
		fprintf(stderr,"Cannot get XCB setup\n");
		return 1;
	}

	if ((xcb_screen=(xcb_setup_roots_iterator(xcb_setup)).data) == NULL) {
		fprintf(stderr,"Cannot get XCB screen\n");
		return 1;
	}

	xcb_window = xcb_generate_id(xcb_connection);

	{
		uint32_t mask;
		uint32_t values[3];

		mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
		values[0] = xcb_screen->white_pixel;
		values[1] =	XCB_EVENT_MASK_EXPOSURE       | XCB_EVENT_MASK_BUTTON_PRESS   |
				XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
				XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW   |
				XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE;
		values[2] = 0;

		xcb_create_window(xcb_connection, /*depth*/0, /*window id*/xcb_window, /*parent window*/xcb_screen->root, /*x*/0, /*y*/ 0,
			640, 480, /*border width*/10, XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb_screen->root_visual, mask, values);
	}

	{
		const char *title = "Hello world";

		xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window,
			XCB_ATOM_WM_NAME, XCB_ATOM_STRING, /*format=*/8/*<-FIXME: What does this mean?? 8 bytes/char?*/,
			strlen(title), title);
	}

	xcb_map_window(xcb_connection, xcb_window);
	xcb_flush(xcb_connection);
	update_xcb_geo();

	{
		uint32_t mask;
		uint32_t values[2];

		mask = XCB_GC_FOREGROUND;
		values[0] = xcb_screen->black_pixel;
		values[1] = 0;

		xcb_gc = xcb_generate_id(xcb_connection);
		xcb_create_gc(xcb_connection, xcb_gc, xcb_window, mask, values);
	}

	init_bitmap();

	/* use xcb_poll_event() if you want to do animation at the same time */
	while ((xcb_event=xcb_wait_for_event(xcb_connection)) != NULL) {
		/* FIXME: can anyone explain why all the XCB samples recommend checking not xcb_event->response_type, but
		 *        instead (xcb_event->response_type & ~0x80)? */
		unsigned char etype = xcb_event->response_type & ~0x80;

		if (etype == XCB_EXPOSE) {
			int pw = xcb_geo->width;
			int ph = xcb_geo->height;

			fprintf(stderr,"Expose event\n");

			update_xcb_geo();

			if (pw != xcb_geo->width || ph != xcb_geo->height) {
				free_bitmap();
				init_bitmap();
				redraw = 1;
			}
		}
		else if (etype == XCB_KEY_PRESS) {
			xcb_key_press_event_t *ev = (xcb_key_press_event_t*)xcb_event;

			/* Okay, here's my biggest gripe about the XCB library. The keyboard events have a vague "detail"
			 * constant that represents the keyboard key, and NOBODY FUCKING DOCUMENTS WHAT IT MEANS OR HOW
			 * TO MAKE SENSE OF IT. That's BAD. Really BAD. Not even Microsoft Windows is that bad for something
			 * so fundamental and basic. For fucks sake MS-DOS and Windows 3.0 had keyboard constants you could
			 * make sense of keyboard input from and that's considered ancient unstable crashy shit in comparison
			 * to Linux/X11. And let me tell you XCB library developers something: A lot of the weird backwards
			 * compat shit that Microsoft had to support in Windows came from such an environment of guessing and
			 * poor documentation. Do you want to maintain a library that must support the guesses of thousands
			 * of developers forever more? Do you want the hell of having to maintain stupid code layouts and
			 * weird values because your poor documentation and lack of constants forced developers to hardcode
			 * key codes? No? Then get your shit straight. */
			if (ev->detail == 9/*FIXME: Where are the keyboard scan code defines???*/) {
				fprintf(stderr,"KEY_PRESS/escape\n");
				free(xcb_event);
				xcb_event = NULL;
				break;
			}
		}

		free(xcb_event);
		xcb_event = NULL;

		if (redraw) {
			redraw = 0;
			xcb_copy_area(xcb_connection, xcb_pixmap, xcb_window, xcb_gc, 0, 0, 0, 0, bitmap_width, bitmap_height);
		}
	}

	freeall();
	xcb_free_gc(xcb_connection,xcb_gc);
	xcb_disconnect(xcb_connection);
	return 0;
}

