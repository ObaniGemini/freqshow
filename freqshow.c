//gcc -lm -lfftw3 -lSDL2 -lSDL2_mixer freqshow.c -o freq

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <fftw3.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>


#define ECHANTILLONS	1024
#define WIDTH			1920
#define HEIGHT			1000
#define PIXELS			( WIDTH * HEIGHT )

#define MIN( a, b )		( a > b ? b : a )
#define MAX( a, b )		( a > b ? a : b )
#define SQUARE( a )		( a * a )

#define COLOR			( rand() % 256 ) | (( rand() % 256 ) << 8) | (( rand() % 256 ) << 16) | ( 255 << 24 )



fftw_complex * infftw = NULL;
fftw_complex * outfftw = NULL; 
fftw_plan planfftw;
int hauteurs1[ECHANTILLONS] = { };
int hauteurs2[ECHANTILLONS] = { };



void drawLine( uint32_t *pixels, int x1, int y1, int x2, int y2 ) {
	int u = x2 - x1, x;
	/* v est la différence en y pour recentrer le segment en 0 côté ordonnées */
	int v = y2 - y1;
	if( y1 == 0 && y2 == 0 )
		return;

	printf("%d %d %d %d\n", x1, y1, x2, y2);

	if(u > v) {
		float p = v / (float)u, y;
		for(x = 0, y = 0; x <= u; x++) {
			y += p;
			pixels[x1 + x + (y1 + ((int)y))*WIDTH] = COLOR;
		}
	} else {
		float p = u / (float)v, y;
		for(x = 0, y = 0; x <= v; x++) {
			y += p;
			pixels[x1 + (int)y + (y1 + x)*WIDTH] = COLOR;
		}
	}
}


void mixCallback(void *udata, Uint8 *stream, int len) {
 	int l = MIN(len >> 1, ECHANTILLONS);
	Sint16 *d = (Sint16 *)stream;

	for( int i = 0; i < l; i++ )
		infftw[i][0] = d[i] / ((1 << 15) - 1.0);

	fftw_execute(planfftw);
	
	for( int i = 0; i < l >> 2; i++ ) {
		int pos = (outfftw[i][0] + outfftw[i][1]) * exp(2.0 * i / (double)(l / 4.0));
		if( pos > 0 ) {
			hauteurs1[i] = pos;
		} else {
			hauteurs2[i] = pos;
		}

		for( int j = 1; j < 4; j++ ) {
			hauteurs1[i + j] = MIN(hauteurs1[i], 255);
			hauteurs2[i + j] = MAX(hauteurs2[i], 0);
		}
	}
}



int main( int argc, char **argv ) {
	if( argc != 2 ) {
		printf("Usage : %s <file_path>\n", argv[0]);
		return 1;
	}
	/*!\brief amplitude des échantillons du signal sonore */
	uint32_t pixels[PIXELS] = { };

	/* préparation des conteneurs de données pour la lib FFTW */
	infftw = fftw_malloc( ECHANTILLONS * sizeof *infftw );
	memset(infftw, 0, ECHANTILLONS * sizeof *infftw);
	outfftw = fftw_malloc( ECHANTILLONS * sizeof *outfftw );
	planfftw = fftw_plan_dft_1d( ECHANTILLONS, infftw, outfftw, FFTW_FORWARD, FFTW_ESTIMATE );


	SDL_Init( SDL_INIT_VIDEO );


	SDL_Event e;
	SDL_Window * window = SDL_CreateWindow( "Freq", 0, 0, WIDTH, HEIGHT, 0 );
	SDL_Renderer * renderer = SDL_CreateRenderer( window, -1, SDL_WINDOW_FULLSCREEN );
	SDL_Texture * texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT );


	Mix_Init( MIX_INIT_OGG );
	Mix_OpenAudio( 44100, AUDIO_S16LSB, 2, ECHANTILLONS );
	Mix_Music * music_playing = Mix_LoadMUS( argv[1] );
	Mix_SetPostMix( mixCallback, NULL );
	Mix_PlayMusic( music_playing, 1 );

	int i = 0;
	for(;;) {
		if (SDL_PollEvent( &e ) && (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)))
			break;

		if( i == 12 ) {
			i = 0;
			memset(pixels, 0, PIXELS * sizeof(uint32_t));
		}

		for( int i = 1; i < ECHANTILLONS; ++i) {
			//pixels[(i * (WIDTH - 1)) / (ECHANTILLONS - 1) + hauteurs1[i]*WIDTH] = ( rand() % 256 ) | (( rand() % 256 ) << 8) | (( rand() % 256 ) << 16) | 255;
			//pixels[PIXELS - (i * (WIDTH - 1)) / (ECHANTILLONS - 1) + hauteurs2[i]*WIDTH] = ( rand() % 256 ) | (( rand() % 256 ) << 8) | (( rand() % 256 ) << 16) | 255;
			drawLine( pixels, ((i-1) * (WIDTH - 1)) / (ECHANTILLONS - 1), hauteurs1[i-1], (i * (WIDTH - 1)) / (ECHANTILLONS - 1), hauteurs1[i] );
		}


		SDL_UpdateTexture( texture, NULL, pixels, WIDTH );
		SDL_RenderCopy( renderer, texture, NULL, NULL );
		SDL_RenderPresent( renderer );
		i++;
	}

	Mix_HaltMusic();
	Mix_FreeMusic(music_playing);
	Mix_CloseAudio();
	Mix_Quit();

	fftw_destroy_plan(planfftw);
	fftw_free(infftw);
	fftw_free(outfftw);

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}