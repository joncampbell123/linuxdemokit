
#include <stdio.h>

#include "SDL2/SDL.h"

SDL_Window*		main_wnd = NULL;
SDL_Renderer*		main_render = NULL;
SDL_Texture*		main_texture = NULL;

int main() {
	SDL_Event event;

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		fprintf(stderr,"SDL_Init failed. Error: %s\n",SDL_GetError());
		return 1;
	}

	if ((main_wnd=SDL_CreateWindow(/*const char *title*/"vhello1",/*x*/SDL_WINDOWPOS_CENTERED,
		/*y*/SDL_WINDOWPOS_CENTERED,/*w (client area?)*/640,/*h (client area?)*/480,
		SDL_WINDOW_SHOWN)) == NULL) {
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

	/* now wait for user input.
	 * note that if you were a demo running animations you might use SDL_PollEvent instead of blocking in SDL_WaitEvent */
	while (1) {
		if (SDL_WaitEvent(&event)) {
			/* SDL_QUIT: Can happen if given a signal like SIGTERM */
			if (event.type == SDL_QUIT) {
				fprintf(stderr,"SDL_QUIT\n");
				break;
			}
			/* can happen if user clicks "close" button in X */
			else if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
					fprintf(stderr,"SDL_WINDOWEVENT/SDL_WINDOWEVENT_CLOSE\n");
					break;
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
		else {
			fprintf(stderr,"SDL_WaitEvent error %s\n",SDL_GetError());
		}
	}

	/* cleanup */
	SDL_DestroyTexture(main_texture);
	SDL_DestroyRenderer(main_render);
	SDL_DestroyWindow(main_wnd);
	SDL_Quit();
	return 0;
}

