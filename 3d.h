#ifndef threed_h
#define threed_h

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include "mathc.h"
#include <vector.h>
#include <map.h>


#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))


typedef struct color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color;

static color white = {255, 255, 255};
static color gray = {128, 128, 128};
static color blue = {0, 0, 255};
static color green = {0, 255, 0};
static color yellow = {255, 255, 0};
static color cyan = {0, 255, 255};
static color red = {255, 0, 0};


typedef struct vertex {
    mfloat_t pos[VEC4_SIZE];
    mfloat_t normal[VEC3_SIZE];
    color colour;
} vertex;

typedef struct face {
    vector vertices;
    int length;
} face;

static const int map_key_size = 64;

typedef struct object {
    vector faces;
    int length;
    map vertices_lis;
} object;

typedef struct light {
    mfloat_t pos[VEC4_SIZE];
    mfloat_t intensity;
} light;

vertex *create_vertex(double x, double y, double z);
vertex *create_vertex_color(double x, double y, double z, color c);
void set_normal(vertex *v, float x, float y, float z);
void free_vertex(vertex *v);
void print_vertex(vertex *v);
double get_vertex_coord(vertex *v, int i);

face *create_face(int length, ...);
int add_vertex_to_face(face *f, vertex *v);
void compute_normal(face *f);
void free_face(face *f);

object *create_object(int length, ...);
int add_face_to_object(object *o, face *f);
int find_vertices(vertex *v, vertex **vl);
int count_vertices(object *o);
void update_vertices_list_from_face(object *o, face *f);
void update_vertices_list(object *o);
void translate_object(object *o, mfloat_t* translation);
void transform_object(object *o, mfloat_t* transformation_matrix);
void free_object(object *object);

light *create_light(float x, float y, float z, float i);
void free_light(light *l);

void print_mat4(mfloat_t *m);
void print_vec4(mfloat_t *m);
void print_vec3(mfloat_t *m);


void create_sphere(object *o, int sectors, int stack, float radius);
void create_obj(object *o, char *filename);


void render_object(SDL_Renderer *r, object *o, mfloat_t *camera, mfloat_t *projection, int w, int h, int only_vertices, int vertice_size);
void render_vertices(SDL_Renderer *r, vertex **vertices, int length, mfloat_t *camera, mfloat_t *projection, int w, int h);
void render_polygon(SDL_Renderer *renderer, mfloat_t **vertices, int length,int height_max,
    int r, int g, int b, int a);

#endif