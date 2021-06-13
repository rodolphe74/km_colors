#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_stdinc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

#include "3d.h"
#include "log.h"
#include "mathc.h"
#include "pixel.h"
#include "jpeg.h"
#include "kmean.h"


#define MAX(x, y) (((x) > (y)) ? (x) : (y))

SDL_Window *window;
SDL_Surface *screen;
SDL_Renderer *renderer;
SDL_Texture *image_texture;


const int w = 640;
const int h = 640;
const int k = 8;
const int max_iter = 100;
int iter_result;


object *colors_object;
object *cube_object;
object *palette_object = NULL;

int current_palette_display = 0;

IMAGE *the_image = NULL;
IMAGE *small_image = NULL;
PALETTE *k_palettes;

mfloat_t angle_step = 0.2;
mfloat_t current_angle = 0.0;

char colors_inf[256];
char k_mean_palette_iteration[256];

//------------------------------------------------------------------------------
// Utils pour le log
//------------------------------------------------------------------------------
static FILE *f;
static void the_log()
{
	f = fopen("km_colors.log", "w");
	log_add_fp(f, LOG_DEBUG);
	log_set_quiet(1);
}
static void end_the_log()
{
	fclose(f);
}


void init() {

	cube_object = create_object(0);
	create_obj(cube_object, "cube.obj");
	
	mfloat_t vec_position[MAT3_SIZE];
	mfloat_t mat_translation[MAT4_SIZE];
	mat4_identity(mat_translation);
	mat4_translation(mat_translation, mat_translation, vec3(vec_position, 0.0, 0.0, 0.0));

}



int create_window()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	window = SDL_CreateWindow("",
				  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				  w, h,
				  0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	return 1;
}



int destroy_window()
{
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 1;
}



SDL_Texture *create_texture_from_image(PIXEL *image_pixels, int width, int height)
{
	int pitch;
	uint8_t *pixels;
	uint8_t *p;

	SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
	SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch);

	for(int y = 0; y < height; y++) {
		Uint32 *p = (Uint32 *)(pixels + pitch*y);
		for(int x = 0; x < width; x++) {
			*(p+x) = SDL_MapRGBA(format, 
				(image_pixels+(y*width)+x)->r, 
				(image_pixels+(y*width)+x)->g, 
				(image_pixels+(y*width)+x)->b, 
				255);
		}
	}

	SDL_UnlockTexture(texture);
	SDL_FreeFormat(format);
	return texture;
}



void update_palette(int idx) {
	int r, g, b;
	double x_coord, y_coord, z_coord;
	color c;

	if (palette_object != NULL) {
		free_object(palette_object);
	}
	palette_object = create_object(0);

	for (int i = 0; i < k_palettes[current_palette_display].size; i++) {
		r = k_palettes[current_palette_display].colors[i][0];
		g = k_palettes[current_palette_display].colors[i][1];
		b = k_palettes[current_palette_display].colors[i][2];
		x_coord = r / 128.0 - 1.0;
		y_coord = g / 128.0 - 1.0;
		z_coord = b / 128.0 - 1.0;
		c.r = r;
		c.g = g;
		c.b = b;
		vertex *v = create_vertex_color(x_coord, y_coord, z_coord, c);
		face *f = create_face(1, v);
		add_face_to_object(palette_object, f);
	}
}


int message_loop()
{

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderClear(renderer);

	mfloat_t position[VEC3_SIZE];
	mfloat_t target[VEC3_SIZE];
	mfloat_t up[VEC3_SIZE];
	mfloat_t view[MAT4_SIZE];
	mat4_look_at(view,
		vec3(position, 0.0, 0.5, 3.0),
		vec3(target, 0.0, 0.0, 0),
		vec3(up, 0.0, 1.0, 0.0));

	mfloat_t perspective[MAT4_SIZE];
	mat4_perspective(perspective, to_radians(90.0), 1, 0.1, 100.0);
	
	mfloat_t mat_y_rotation_cube[MAT4_SIZE];
	mat4_identity(mat_y_rotation_cube);
	mat4_rotation_y(mat_y_rotation_cube, to_radians(0.2));


	int exit = 0;
	while (!exit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {

			if (event.type == SDL_QUIT) {
				exit = 1;
			}

			switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						exit = 1;
					}

					if (event.key.keysym.sym == SDLK_KP_PLUS) {
						current_palette_display = (current_palette_display >= iter_result - 1? iter_result - 1 : ++current_palette_display);
						sprintf(k_mean_palette_iteration, "k-mean iteration: %d/%d  (+/-)", current_palette_display + 1, iter_result);
						update_palette(current_palette_display);

						mfloat_t mat_y[MAT4_SIZE];
						mat4_identity(mat_y);
						mat4_rotation_y(mat_y, to_radians(current_angle));
						transform_object(palette_object, mat_y);
					}

					if (event.key.keysym.sym == SDLK_KP_MINUS) {
						current_palette_display = (current_palette_display == 0 ? 0 : current_palette_display--);
						current_palette_display = (current_palette_display <= 0? 0 : --current_palette_display);
						sprintf(k_mean_palette_iteration, "k-mean iteration: %d/%d  (+/-)", current_palette_display + 1, iter_result);
						update_palette(current_palette_display);

						mfloat_t mat_y[MAT4_SIZE];
						mat4_identity(mat_y);
						mat4_rotation_y(mat_y, to_radians(current_angle));
						transform_object(palette_object, mat_y);
					}

					break;
				default:
					break;
				}
		}
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);

		render_object(renderer, cube_object, view, perspective, w, h, 0, 1);
		render_object(renderer, cube_object, view, perspective, w, h, 1, 1);

		render_object(renderer, colors_object, view, perspective, w, h, 1, 1);

		render_object(renderer, palette_object, view, perspective, w, h, 1, 5);


		stringRGBA(renderer, 10, 10, colors_inf, 255, 255, 255, 255);
		stringRGBA(renderer, 10, 22, k_mean_palette_iteration, 255, 255, 255, 255);

		SDL_RenderCopy(renderer, image_texture, NULL, 
			&(SDL_Rect){w - small_image->width, h - small_image->height, small_image->width, small_image->height});

		SDL_RenderPresent(renderer);
		transform_object(cube_object, mat_y_rotation_cube);
		transform_object(colors_object, mat_y_rotation_cube);
		transform_object(palette_object, mat_y_rotation_cube);
		current_angle += 0.2;
		
		SDL_Delay(5);
	}

	free_object(cube_object);
	free_object(colors_object);
	free_object(palette_object);
	SDL_DestroyTexture(image_texture);
	destroy_window();

}



int main(int argc, char *argv[])
{
	the_log();

	int threshold = 20;
	int count_24;
	int count = 0;
	int r, g, b;
	double x_coord, y_coord, z_coord;
	color c;
	unsigned int *first_key, *last_key, *current_key;
	k_palettes = malloc(max_iter * sizeof(PALETTE));


	if (argc < 2) {
		printf("Image jpg en argument");
		return 0;
	}

	if (argc >= 3) {
		threshold = atoi(argv[2]);
		if (threshold == 0) {
			printf("Seuil par défaut à 20\n");
			threshold = 20;
		}
	} else {
		printf("Seuil par défaut à 20\n");
	}

	if (!create_window()) {
		printf("Impossible de creer l'ecran\n");
		return 0;
	}

	init();


	the_image = load(argv[1]);
	if (!the_image)
		return 0;


	float ratio = 1.0;
	float ratio_x, ratio_y;
	if (the_image->width >= 160) {
		ratio_x = the_image->width / 160.0;
		printf("ratio x %f\n", ratio_x);
	}
	if (the_image->height > 100) {
		ratio_y = the_image->height / 100.0;
		printf("ratio y %f\n", ratio_y);
	}
	ratio = MAX(ratio_x, ratio_y);
	small_image = bilinear_resize(the_image,
						    (int)the_image->width / ratio,
						    (int)the_image->height / ratio);
	image_texture = create_texture_from_image(small_image->pixels, small_image->width, small_image->height);


	colors_object = create_object(0);
	palette_object = create_object(0);

	map the_colors;
	get_colors_map(the_image, &the_colors);
	size_t colors_count = map_size(the_colors);
	printf("Nombre de couleurs: %d\n", colors_count);

	iter_result = guess_palette_kmean(the_image, k_palettes, k, max_iter);
	current_palette_display = iter_result - 1;
	sprintf(k_mean_palette_iteration, "k-mean iteration: %d/%d  (+/-)", current_palette_display + 1, iter_result);
	update_palette(current_palette_display);

	first_key = map_first(the_colors);
	last_key = map_last(the_colors);

	current_key = first_key;
	while (current_key != NULL) {
		map_get(&count_24, the_colors, current_key);
		// printf("get %u -> %u:%d\n", color_index, *current_key, count_24);

		r = (*current_key) / 65536;
		g = ((*current_key) % 65536) / 256;
		b = ((*current_key) % 65536) % 256;

		if (count_24 >= threshold) {
			x_coord = r / 128.0 - 1.0;
			y_coord = g / 128.0 - 1.0;
			z_coord = b / 128.0 - 1.0;
			c.r = r;
			c.g = g;
			c.b = b;

			vertex *v = create_vertex_color(x_coord, y_coord, z_coord, c);
			face *f = create_face(1, v);
			add_face_to_object(colors_object, f);

			count++;
		}

		current_key = map_higher(the_colors, current_key);
	}
	printf("couleurs affichees: %d\n", count);

	fflush(stdout);

	sprintf(colors_inf, "%d couleurs affichees sur %d au total", count, colors_count);
	message_loop();

	map_destroy(the_colors);
	free_image(the_image);
	free_image(small_image);
	free(k_palettes);

	end_the_log();

	return 0;
}
