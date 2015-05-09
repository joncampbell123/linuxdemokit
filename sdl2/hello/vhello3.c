
#include <stdio.h>

#include "SDL2/SDL.h"

SDL_Window*		main_wnd = NULL;
SDL_Renderer*		main_render = NULL;
SDL_Texture*		main_texture = NULL;

int main() {
	/* animation */
	double motion_a = 0,motion_adelta = 1,motion_base = 0;
	SDL_Rect rfirst_from,rfirst_to;
	SDL_Rect rlast_from,rlast_to;
	int tex_w = 0,tex_h = 0;
	int wnd_w = 0,wnd_h = 0;
	int redraw = 1;

	Uint32 timebase = 0,timenow,drawnext = 0,time_motion_change = 0; /* in milliseconds */
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
	 * SDL2 has a builtin BMP loader.
	 * we have to load the BMP to a surface, then to a texture.
	 * once it's a texture we throw away the surface. */
	{
		SDL_Surface *surf;

		if ((surf=SDL_LoadBMP("../../res/unhappykitten800.bmp")) == NULL) {
			fprintf(stderr,"SDL_LoadBMP failed. Error: %s\n",SDL_GetError());
			SDL_Quit();
			return 1;
		}

		if ((main_texture=SDL_CreateTextureFromSurface(main_render,surf)) == NULL) {
			fprintf(stderr,"SDL_CreateTextureFromSurface failed. Error: %s\n",SDL_GetError());
			SDL_Quit();
		}

		SDL_FreeSurface(surf);
	}

	/* show it */
	SDL_RenderClear(main_render);
	SDL_RenderCopy(main_render,main_texture,NULL,NULL);
	SDL_RenderPresent(main_render);

	/* decide the two extremes we will animate between */
	{
		Uint32 fmt;
		int acc;

		SDL_QueryTexture(main_texture,&fmt,&acc,&tex_w,&tex_h);
		SDL_GetWindowSize(main_wnd,&wnd_w,&wnd_h);
		redraw = 1;
	}

	/* we're a demo running animation. use SDL_WaitEventTimeout to balance running animation with CPU load */
	timebase = SDL_GetTicks();
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
			SDL_Rect rfinal_from,rfinal_to;
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

				SDL_RenderClear(main_render);
				SDL_RenderCopy(main_render,main_texture,&rfinal_from,&rfinal_to);
				SDL_RenderPresent(main_render);
			}
		}
	}

	/* cleanup */
	SDL_DestroyTexture(main_texture);
	SDL_DestroyRenderer(main_render);
	SDL_DestroyWindow(main_wnd);
	SDL_Quit();
	return 0;
}

