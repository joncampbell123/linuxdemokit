
#include <stdio.h>
#include <errno.h>

#include <png.h>
#include <jpeglib.h>

#include "SDL2/SDL.h"

SDL_Window*		main_wnd = NULL;
SDL_Renderer*		main_render = NULL;
SDL_Texture*		main_texture = NULL;
SDL_Texture*		lame_texture = NULL;

int main() {
	/* animation */
	double motion_a = 0,motion_adelta = 1,motion_base = 0;
	SDL_Rect rfirst_from,rfirst_to;
	SDL_Rect rlast_from,rlast_to;
	int lame_w = 0,lame_h = 0;
	int tex_w = 0,tex_h = 0;
	int wnd_w = 0,wnd_h = 0;
	int redraw = 1;

	Uint32 timebase = 0,timenow,drawnext = 0,time_motion_change = 0,lame_timebase = 0; /* in milliseconds */
	int demo_fps = 60;
	SDL_Event event;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr,"SDL_Init failed. Error: %s\n",SDL_GetError());
		return 1;
	}

	if ((main_wnd=SDL_CreateWindow(/*const char *title*/"vhello1",/*x*/SDL_WINDOWPOS_CENTERED,
		/*y*/SDL_WINDOWPOS_CENTERED,/*w (client area?)*/640,/*h (client area?)*/480,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)) == NULL) {
		fprintf(stderr,"SDL_CreateWindow failed. Error: %s\n",SDL_GetError());
		SDL_Quit();
		return 1;
	}

	/* as a habit I generally do not assume vsync or hardware acceleration is available */
	main_render=SDL_CreateRenderer(main_wnd, /*index*/-1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (main_render == NULL)
		main_render=SDL_CreateRenderer(main_wnd, /*index*/-1, SDL_RENDERER_PRESENTVSYNC);
	if (main_render == NULL)
		main_render=SDL_CreateRenderer(main_wnd, /*index*/-1, SDL_RENDERER_ACCELERATED);
	if (main_render == NULL)
		main_render=SDL_CreateRenderer(main_wnd, /*index*/-1, 0); /* :( */
	if (main_render == NULL) {
		fprintf(stderr,"SDL_CreateRenderer failed. Error: %s\n",SDL_GetError());
		SDL_Quit();
		return 1;
	}

	/* now give it something to show.
	 * this time we use libjpeg to load a JPEG image. */
	{
		SDL_Surface *surf;
		struct jpeg_decompress_struct decom;
		struct jpeg_error_mgr errmgr;
		FILE *fp;

		if ((fp=fopen("../../res/kitten1280x800.jpg","rb")) == NULL) {
			fprintf(stderr,"Failed to open kitten1280x800.jpg, %s\n",strerror(errno));
			SDL_Quit();
			return 1;
		}
		decom.err = jpeg_std_error(&errmgr);
		jpeg_create_decompress(&decom);
		jpeg_stdio_src(&decom,fp);
		jpeg_read_header(&decom,1);
		if (decom.image_width == 0 || decom.image_height == 0) {
			fprintf(stderr,"JPEG image has no dimensions\n");
			SDL_Quit();
			return 1;
		}

		/* libjpeg can decompress to 24-bit RGB */
		if ((surf=SDL_CreateRGBSurface(0,decom.image_width,decom.image_height,24,0x000000FF,0x0000FF00,0x00FF0000,0)) == NULL) {
			fprintf(stderr,"SDL_CreateRGBSurface failed. Error: %s\n",SDL_GetError());
			SDL_Quit();
			return 1;
		}

		/* convert to RGB please */
		decom.out_color_space = JCS_RGB;
		jpeg_start_decompress(&decom);

		{
			JSAMPARRAY rows = (JSAMPROW*)malloc(sizeof(JSAMPROW) * decom.image_height);
			int y;

			for (y=0;y < decom.image_height;y++)
				rows[y] = (char*)surf->pixels + (y * surf->pitch);

			while (decom.output_scanline < decom.image_height)
				jpeg_read_scanlines(&decom,rows+decom.output_scanline,decom.image_height-decom.output_scanline);

			free(rows);
		}

		jpeg_finish_decompress(&decom);
		jpeg_destroy_decompress(&decom);
		fclose(fp);

		if ((main_texture=SDL_CreateTextureFromSurface(main_render,surf)) == NULL) {
			fprintf(stderr,"SDL_CreateTextureFromSurface failed. Error: %s\n",SDL_GetError());
			SDL_Quit();
		}

		tex_w = surf->w;
		tex_h = surf->h;
		SDL_FreeSurface(surf);
	}

	/* another texture, for humor.
	 * this time, we use libpng and create the surface ourself. */
	{
		SDL_Surface *surf;
		png_structp png_ptr;
		png_infop png_info;
		png_bytep *rows;
		int w,h,y;
		FILE *fp;

		if ((fp=fopen("../../res/lame.png","rb")) == NULL) {
			fprintf(stderr,"fopen failed, %s\n",strerror(errno));
			SDL_Quit();
			return 1;
		}

		if ((png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL)) == NULL) {
			fprintf(stderr,"png create struct failed\n");
			SDL_Quit();
			return 1;
		}

		if ((png_info=png_create_info_struct(png_ptr)) == NULL) {
			fprintf(stderr,"png create info struct failed\n");
			SDL_Quit();
			return 1;
		}

		png_init_io(png_ptr,fp);
		png_read_info(png_ptr,png_info);
		w = png_get_image_width(png_ptr,png_info);
		h = png_get_image_height(png_ptr,png_info);

		if (w == 0 || h == 0) {
			fprintf(stderr,"PNG with no dimensions\n");
			SDL_Quit();
			return 1;
		}

		/* we assume PNG with alpha, RGB, 32bpp */
		if ((surf=SDL_CreateRGBSurface(0,w,h,32,0x000000FF,0x0000FF00,0x00FF0000,0xFF000000)) == NULL) {
			fprintf(stderr,"SDL_CreateRGBSurface failed. Error: %s\n",SDL_GetError());
			SDL_Quit();
			return 1;
		}

		rows = (png_bytep*)malloc(sizeof(png_bytep) * h);
		for (y=0;y < h;y++) rows[y] = (char*)surf->pixels + (y * surf->pitch);
		png_read_image(png_ptr,rows);
		free(rows);

		if (setjmp(png_jmpbuf(png_ptr))) {
			fprintf(stderr,"PNG read failed\n");
			SDL_Quit();
		}

		png_destroy_read_struct(&png_ptr,&png_info,NULL);

		fclose(fp);

		if ((lame_texture=SDL_CreateTextureFromSurface(main_render,surf)) == NULL) {
			fprintf(stderr,"SDL_CreateTextureFromSurface failed. Error: %s\n",SDL_GetError());
			SDL_Quit();
		}

		lame_w = surf->w;
		lame_h = surf->h;
		SDL_SetTextureBlendMode(lame_texture,SDL_BLENDMODE_BLEND);
		SDL_FreeSurface(surf);
	}

	/* show it */
	SDL_RenderClear(main_render);
	SDL_RenderCopy(main_render,main_texture,NULL,NULL);
	SDL_RenderPresent(main_render);

	/* decide the two extremes we will animate between */
	{
		SDL_GetWindowSize(main_wnd,&wnd_w,&wnd_h);
		redraw = 1;
	}

	/* we're a demo running animation. use SDL_WaitEventTimeout to balance running animation with CPU load */
	timebase = SDL_GetTicks();
	lame_timebase = SDL_GetTicks();
	while (1) {
		if (SDL_WaitEventTimeout(&event,1000 / demo_fps)) {
			/* SDL_QUIT: Can happen if given a signal like SIGTERM */
			if (event.type == SDL_QUIT) {
				fprintf(stderr,"SDL_QUIT\n");
				break;
			}
			else if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_CLOSE) { /* User clicks the "X" in the Window Manager or closes the window somehow */
					fprintf(stderr,"SDL_WINDOWEVENT/SDL_WINDOWEVENT_CLOSE\n");
					break;
				}
				else if (event.window.event == SDL_WINDOWEVENT_RESIZED) { /* User resizes the window */
					fprintf(stderr,"SDL_WINDOWEVENT/SDL_WINDOWEVENT_RESIZED\n");
					SDL_GetWindowSize(main_wnd,&wnd_w,&wnd_h);
					redraw = 1;
				}
			}
			/* allow user to hit ESC to quit */
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.state == SDL_PRESSED && event.key.keysym.sym == SDLK_ESCAPE) {
					fprintf(stderr,"SDL_KEYDOWN/SDLK_ESCAPE\n");
					break;
				}
			}
		}

		timenow = SDL_GetTicks();
		if (timenow >= drawnext) {
			double p_motion_a = motion_a;

			/* animate */
			if (timebase == 0) {
				timebase = timenow;
				time_motion_change = timenow + 4000;
			}

			motion_a = motion_base + ((((double)(timenow - timebase)) / (1000 * 2)) * motion_adelta);
			if (motion_a >= 1.0 && motion_adelta > 0) {
				motion_a = 1.0;
				motion_base = 1.0;
				motion_adelta = 0;
				timebase = timenow;
				time_motion_change = timenow + 2000;
			}
			else if (motion_a <= 0 && motion_adelta < 0) {
				motion_a = 0.0;
				motion_base = 0.0;
				motion_adelta = 0;
				timebase = timenow;
				time_motion_change = timenow + 2000;
			}
			else if (timenow >= time_motion_change) {
				time_motion_change = timenow + 4000;
				timebase = timenow;
				if (motion_a >= 0.5)
					motion_adelta = -1.0;
				else
					motion_adelta = 1.0;
			}

			if (motion_a != p_motion_a)
				redraw = 1;

			if ((drawnext += (1000 / demo_fps)) < timenow)
				drawnext = timenow + (1000 / demo_fps);

			/* need to redraw */
			if (redraw) {
				SDL_Rect rfinal_from,rfinal_to,r_lame;

				redraw = 0;

				/* from: full texture scaled to window */
				rfirst_from.x = 0;	rfirst_from.y = 0;
				rfirst_from.w = tex_w;	rfirst_from.h = tex_h;
				rfirst_to.x = 0;	rfirst_to.y = 0;
				rfirst_to.w = wnd_w;	rfirst_to.h = wnd_h;

				/* to: 1:1 texture scale to window, centered. */
				rlast_from.x = 0;
				rlast_to.x = (wnd_w - tex_w) / 2;
				rlast_from.y = 0;
				rlast_to.y = (wnd_h - tex_h) / 2;
				rlast_from.w = tex_w;
				rlast_to.w = tex_w;
				rlast_from.h = tex_h;
				rlast_to.h = tex_h;

				/* and the scale is...? */
				rfinal_from.x = rfirst_from.x + (int)((rlast_from.x - rfirst_from.x) * motion_a);
				rfinal_from.y = rfirst_from.y + (int)((rlast_from.x - rfirst_from.y) * motion_a);
				rfinal_from.w = rfirst_from.w + (int)((rlast_from.w - rfirst_from.w) * motion_a);
				rfinal_from.h = rfirst_from.h + (int)((rlast_from.h - rfirst_from.h) * motion_a);
				rfinal_to.x = rfirst_to.x + (int)((rlast_to.x - rfirst_to.x) * motion_a);
				rfinal_to.y = rfirst_to.y + (int)((rlast_to.y - rfirst_to.y) * motion_a);
				rfinal_to.w = rfirst_to.w + (int)((rlast_to.w - rfirst_to.w) * motion_a);
				rfinal_to.h = rfirst_to.h + (int)((rlast_to.h - rfirst_to.h) * motion_a);

				/* and the humor */
				{
					int anim_x = ((timenow - lame_timebase) * lame_w) / 1000;

					if (anim_x > lame_w) anim_x = lame_w;

					r_lame.x = wnd_w - anim_x;
					r_lame.y = ((wnd_h - lame_h) * 7) / 8;
					r_lame.w = lame_w;
					r_lame.h = lame_h;
				}

				SDL_RenderClear(main_render);

				SDL_SetRenderDrawBlendMode(main_render,SDL_BLENDMODE_NONE);
				SDL_RenderCopy(main_render,main_texture,&rfinal_from,&rfinal_to);

				SDL_SetRenderDrawBlendMode(main_render,SDL_BLENDMODE_BLEND);
				SDL_RenderCopy(main_render,lame_texture,NULL,&r_lame);

				SDL_RenderPresent(main_render);
			}
		}
	}

	/* cleanup */
	SDL_DestroyTexture(lame_texture);
	SDL_DestroyTexture(main_texture);
	SDL_DestroyRenderer(main_render);
	SDL_DestroyWindow(main_wnd);
	SDL_Quit();
	return 0;
}

