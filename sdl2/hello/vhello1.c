
#include <stdio.h>

#include "SDL2/SDL.h"

SDL_Window*		main_wnd = NULL;
SDL_Renderer*		main_render = NULL;

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
	SDL_DestroyRenderer(main_render);
	SDL_DestroyWindow(main_wnd);
	SDL_Quit();
	return 0;
}

