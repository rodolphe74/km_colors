#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3d.h"
#include "log.h"
#include "mathc.h"


static int compare_c_string(const void *const one, const void *const two)
{
	const unsigned char *a = (char *)one;
	const unsigned char *b = (char *)two;

	return strcmp(a, b);
}


vertex *create_vertex(double x, double y, double z) {
    vertex *v = malloc(sizeof(vertex));
    v->pos[0] = x;
    v->pos[1] = y;
    v->pos[2] = z;
    v->pos[3] = 1;
    v->colour = white;
    return v;
}

vertex *create_vertex_color(double x, double y, double z, color c) {
    vertex *v = create_vertex(x, y, z);
    v->colour = c;
    return v;
}

void set_normal(vertex *v, float x, float y, float z) {
    v->normal[0] = x;
    v->normal[1] = y;
    v->normal[2] = z;
}


double get_vertex_coord(vertex *v, int i) {
    return v->pos[i];
}

void print_vertex(vertex *v) {
    printf(">[%lf %lf %lf %lf]\n", get_vertex_coord(v, 0), get_vertex_coord(v, 1), get_vertex_coord(v, 2), get_vertex_coord(v, 3));
}

void free_vertex(vertex *v) {
    free(v);
}


face *create_face(int length, ...) {
    face *f = malloc(sizeof(face));
    f->length = length;
    f->vertices = vector_init(sizeof(vertex *));
    va_list valist;
    va_start(valist, length);
    for (int i = 0; i < length; i++) {
        vertex *v = va_arg(valist, vertex*);
        vector_add_last(f->vertices, &(v));
    }
    va_end(valist);
    return f;
}

int add_vertex_to_face(face *f, vertex *v) {
    if (f->vertices == NULL) {
        return 0;
    }
    vector_add_last(f->vertices, &(v));
    f->length ++;
    return 1;
}

void compute_normal(face *f) {
    if (f->length > 1) {

        mfloat_t v1[3];
        mfloat_t v2[3];
        mfloat_t *c = malloc(sizeof(mfloat_t) * VEC3_SIZE);

        vertex *vx0, *vx1, *vxn;
        vector_get_at(&vx0, f->vertices, 0);
        vector_get_at(&vx1, f->vertices, 1);
        vector_get_at(&vxn, f->vertices, f->length - 1);

        vec3_subtract(v1, vx1->pos, vx0->pos);
        vec3_subtract(v2, vxn->pos, vx0->pos);

        vec3_cross(c, v1, v2);

        vertex *v;
        for (int i = 0; i < f->length; i++) {
            vector_get_at(&v, f->vertices, i);
            set_normal(v, c[0], c[1], -c[2]);
        }
        free(c);
    }
}

void free_face(face *f) {
    vertex *v;
    for (int i = 0; i < f->length; i++) {
        vector_get_at(&v, f->vertices, i);
        free_vertex(v);
    }
    vector_destroy(f->vertices);
    free(f);
}


object *create_object(int length, ...) {
    object *o = malloc(sizeof(object));
    o->length = length;
    o->faces = vector_init(sizeof(face *));
    // o->vertices_list = NULL;
    o->vertices_lis = map_init(map_key_size, sizeof(vertex *), compare_c_string);
    va_list valist;
    va_start(valist, length);
    for (int i = 0; i < length; i++) {
        face *f = va_arg(valist, face *);
        vector_add_last(o->faces, &(f));
    }
    va_end(valist);
    update_vertices_list(o);
    
    return o;
}


int add_face_to_object(object *o, face *f) {
    if (o->faces == NULL) {
        return 0;
    }
    vector_add_last(o->faces, &(f));
    o->length ++;

    // update_vertices_list(o);
    update_vertices_list_from_face(o, f);
    return 1;
}

void update_vertices_list_from_face(object *o, face *f)
{
    char address[map_key_size];
    vertex *v, *vf;
    for (int j = 0; j < f->length; j++) {
        vector_get_at(&v, f->vertices, j);

        sprintf(address, "%p", v);
        int inside = map_get(&vf, o->vertices_lis, address);
        if (!inside) {
            map_put(o->vertices_lis, address, &(v));
        }

    }
}

void update_vertices_list(object *o) {
    map_clear(o->vertices_lis);
    char address[map_key_size];
    vertex *v, *vf;
    for (int i = 0; i < o->length; i++) {
        face *f;
        vector_get_at(&f, o->faces, i);
        for (int j = 0; j < f->length; j++) {
            vector_get_at(&v, f->vertices, j);
            sprintf(address, "%p", v);
            int inside = map_get(&vf, o->vertices_lis, address);
            if (!inside) {
                printf("$ not inside");
                map_put(o->vertices_lis, address, &(v));
            }
        }
    }
}

void translate_object(object *o, mfloat_t* translation) {
    char *current_key = map_first(o->vertices_lis);
    vertex *v;
    while (current_key != NULL) {
        map_get(&v, o->vertices_lis, current_key);
        vec4_multiply_mat4(v->pos, v->pos, translation);
        current_key = map_higher(o->vertices_lis, current_key);
    }
}

void transform_object(object *o, mfloat_t* transformation_matrix) {
    fflush(stdout);
    char *current_key = map_first(o->vertices_lis);
    vertex *v;
    fflush(stdout);
    while (current_key != NULL) {
        map_get(&v, o->vertices_lis, current_key);
        vec4_multiply_mat4(v->pos, v->pos, transformation_matrix);
        current_key = map_higher(o->vertices_lis, current_key);
    }
}

void free_object(object *o) {
    char *current_key = map_first(o->vertices_lis);
    vertex *v;
    while (current_key != NULL) {
        map_get(&v, o->vertices_lis, current_key);
        free_vertex(v);
        current_key = map_higher(o->vertices_lis, current_key);
    }

    // Suppression des faces
    for (int i = 0; i < o->length; i++) {
        face *f;
        vector_get_at(&f, o->faces, i);
        free(f);
    }
    vector_destroy(o->faces);

    // Suppression de la liste
    map_destroy(o->vertices_lis);

    // Suppression de l'objet
    free(o);
}



void create_sphere(object *o, int sectors, int stacks, float radius) {

    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, length_inv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex texCoord

    float sector_step = 2 * M_PI / sectors;
    float stack_step = M_PI / stacks;
    float sector_angle, stack_angle;

    vertex **vertices = malloc((sectors + 1) * (stacks + 1) * sizeof(vertex*));

    int count = 0;
    for(int i = 0; i <= stacks; ++i)
    {
        stack_angle = M_PI / 2 - i * stack_step;     // starting from pi/2 to -pi/2
        xy = radius * cosf(stack_angle);             // r * cos(u)
        z = radius * sinf(stack_angle);              // r * sin(u)

        // add (sectorCount+1) vertices per stack
        // the first and last vertices have same position and normal, but different tex coords
        for(int j = 0; j <= sectors; ++j)
        {
            sector_angle = j * sector_step;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sector_angle);             // r * cos(u) * cos(v)
            y = xy * sinf(sector_angle);             // r * cos(u) * sin(v)
            vertices[count] = create_vertex(x, y, z);

            // normalized vertex normal (nx, ny, nz)
            nx = x * length_inv;
            ny = y * length_inv;
            nz = z * length_inv;
            // set_normal(vertices[count], nx, ny, nz);

            count++;
        }
    }

    // generate CCW index list of sphere triangles
    // k1--k1+1
    // |  / |
    // | /  |
    // k2--k2+1
    int k1, k2;
    for(int i = 0; i < stacks; ++i)
    {
        k1 = i * (sectors + 1);     // beginning of current stack
        k2 = k1 + sectors + 1;      // beginning of next stack

        for(int j = 0; j < sectors; ++j, ++k1, ++k2)
        {

            if(i != 0)
            {
                face *f = create_face(3, vertices[k1], vertices[k2], vertices[k1 + 1]);
                add_face_to_object(o, f);
                compute_normal(f);
            }

            // k1+1 => k2 => k2+1
            if(i != (stacks-1))
            {
                face *f = create_face(3, vertices[k1 + 1], vertices[k2], vertices[k2 + 1]);
                add_face_to_object(o, f);
                compute_normal(f);
            }
        }

    }
    update_vertices_list(o);
}


void create_obj(object *o, char *filename) {
    FILE* filePointer;
    int bufferLength = 1024;
    char buffer[bufferLength];
    char header[5];
    int vertices_count = 0;
    int normals_count = 0;
    int i = 0;

    filePointer = fopen(filename, "r");
    while(fgets(buffer, bufferLength, filePointer)) {
        sscanf(buffer, "%s " , header);
        if (strcmp("v", header) == 0) {
            vertices_count ++;
        }
        if (strcmp("vn", header) == 0) {
            normals_count ++;
        }
    }
    fclose(filePointer);

    printf("vertices count :%d\n", vertices_count);
    vertex **vertex_list = malloc (sizeof(vertex *) * vertices_count);

    printf("normals count :%d\n", normals_count);
    mfloat_t **normals_list = malloc (sizeof(mfloat_t *) * normals_count);

    filePointer = fopen(filename, "r");
    while(fgets(buffer, bufferLength, filePointer)) {
        sscanf(buffer, "%s " , header);
        if (strcmp("v", header) == 0) {
            float x, y, z;
            sscanf(buffer, "%s %f %f %f" , header, &x, &y, &z);
            vertex *v = create_vertex(x, y, z);
            vertex_list[i] = v;
            i++;
        }

    }
    fclose(filePointer);

    i = 0;
    filePointer = fopen(filename, "r");
    while(fgets(buffer, bufferLength, filePointer)) {
        sscanf(buffer, "%s " , header);
        if (strcmp("vn", header) == 0) {
            float x, y, z;
            sscanf(buffer, "%s %f %f %f" , header, &x, &y, &z);
            // printf("vn %f %f %f\n", x, y, z);
            mfloat_t *n = malloc(sizeof(mfloat_t) * 3);
            n[0] = x;
            n[1] = y;
            n[2] = z;
            normals_list[i] = n;
            i++;
        }
    }
    fclose(filePointer);

    filePointer = fopen(filename, "r");
    int face_indexes[10];
    int normal_indexes[10];
    int k;
    while(fgets(buffer, bufferLength, filePointer)) {
        sscanf(buffer, "%s " , header);
        if (strcmp("f", header) == 0) {
            i = 0;
            k = 0;
            char *token;
            const char s[2] = " ";
            token = strtok(buffer, s);
            while( token != NULL ) {
                if (i>0) {
                    // printf( "token %d %s\n", i, token );
                    int i, j;
                    sscanf(token, "%d//%d", &i, &j);
                    // printf("   %d %d\n", i, j);
                    face_indexes[k] = i;
                    normal_indexes[k] = j;
                    k++;
                }
                token = strtok(NULL, s);
                i++;
            }
            // printf("k %d\n", k);
            face *f = create_face(0);
            for (i = 0; i < k; i++) {
                // printf("face_indexes[%d]=%d\n", i, face_indexes[i]);
                vertex *v = create_vertex(vertex_list[face_indexes[i] - 1]->pos[0], 
                    vertex_list[face_indexes[i] - 1]->pos[1], 
                    vertex_list[face_indexes[i] - 1]->pos[2]);
                // print_vertex(v);
                add_vertex_to_face(f, v);
                // printf("normal_indexes[%d]=%d\n", i, normal_indexes[i]);
                set_normal(v, normals_list[normal_indexes[i] - 1][0], 
                    normals_list[normal_indexes[i] - 1][1], 
                    normals_list[normal_indexes[i] - 1][2]);
                //  print_vec3(v->normal);
            }
            // printf("len %d\n", f->length);
            add_face_to_object(o, f);
        }
    }
    fclose(filePointer);


    free(vertex_list);
    for (int i = 0; i < normals_count; i++) {
        free(normals_list[i]);
    }
    free(normals_list);

    update_vertices_list(o);
}


void print_mat4(mfloat_t *m) {
    printf("[ %f %f %f %f\n", m[0], m[4], m[8], m[12]);
    printf("  %f %f %f %f\n", m[1], m[5], m[9], m[13]);
    printf("  %f %f %f %f\n", m[2], m[6], m[10], m[14]);
    printf("  %f %f %f %f  ]\n", m[3], m[7], m[11], m[15]);
}

void print_vec4(mfloat_t *v) {
    printf("[ ");
    for (int i = 0; i < VEC4_SIZE; i++) {
        printf("%f ", v[i]);
    }
    printf("]\n");
}

void print_vec3(mfloat_t *v) {
    printf("[ ");
    for (int i = 0; i < VEC3_SIZE; i++) {
        printf("%f ", v[i]);
    }
    printf("]\n");
}


void render_object(SDL_Renderer *r, object *o, mfloat_t *camera, mfloat_t *projection, int w, int h, int only_vertices, int vertice_size) {

    for (int i = 0; i < o->length; i++) {

        face *f;
        vector_get_at(&f, o->faces, i);

        int paint_polygon = 1;
        mfloat_t center_face[VEC3_SIZE];
        mfloat_t normal_face[VEC3_SIZE];
        vec3(center_face, 0, 0, 0);
        vec3(normal_face, 0, 0, 0);
        Sint16 *xs = malloc(sizeof(Sint16) * f->length);
        Sint16 *ys = malloc(sizeof(Sint16) * f->length);
        color c;
        vertex *world_vertex;

		for (int j = 0; j < f->length; j++) {

            vector_get_at(&world_vertex, f->vertices, j);

			float_t *world_pos = world_vertex->pos;
            float_t *world_norm = world_vertex->normal;
            vec3_add(center_face, center_face, world_pos);
            vec3_add(normal_face, normal_face, world_norm);

			float_t camera_pos[4];
			vec4_multiply_mat4(camera_pos, world_pos, camera);

			float_t projection_pos[4];
			vec4_multiply_mat4(projection_pos, camera_pos, projection);

			vec4_divide_f(projection_pos, projection_pos, projection_pos[3]);
            
            if (projection_pos[0] < -1 
                || projection_pos[0] > 1 
                || projection_pos[1] < -1 
                || projection_pos[1] > 1)
                paint_polygon = 0;
                    
            // creation du polygon x,y projete et z non projete
            mfloat_t *vertex_poly = malloc(3 * sizeof(mfloat_t));
            vertex_poly[0] = MIN(w - 1, (uint32_t)((projection_pos[0] + 1) * 0.5 * w));
            vertex_poly[1] = MIN(h - 1, (uint32_t)((1 - (projection_pos[1] + 1) * 0.5) * h));
            vertex_poly[2] = camera_pos[2];
            // polygon_proj[j] = vertex_poly;
            xs[j] = vertex_poly[0];
            ys[j] = vertex_poly[1];
            c = world_vertex->colour;

            if (only_vertices && paint_polygon) {
                if (vertice_size <= 1) {
                    pixelRGBA(r, xs[j], ys[j], world_vertex->colour.r, world_vertex->colour.g, world_vertex->colour.b, 255);
                } else {
                    filledCircleRGBA(r, xs[j], ys[j], vertice_size, world_vertex->colour.r, world_vertex->colour.g, world_vertex->colour.b, 255);
                    aacircleRGBA(r, xs[j], ys[j], vertice_size, 255, 255, 255, 255);
                }
            }
            free(vertex_poly);
		}

        if (paint_polygon && !only_vertices) {
            aapolygonRGBA(r, xs, ys, f->length, 64, 64, 64, 128);
        }

        free(xs);
        free(ys);
	}
}


void render_vertices(SDL_Renderer *r, vertex **vertices, int length, mfloat_t *camera, mfloat_t *projection, int w, int h) {
    for (int j = 0; j < length; j++) {
        vertex *world_vertex = vertices[j];
        float_t *world_pos = world_vertex->pos;

        float_t camera_pos[4];
        vec4_multiply_mat4(camera_pos, world_pos, camera);

        float_t projection_pos[4];
        vec4_multiply_mat4(projection_pos, camera_pos, projection);

        vec4_divide_f(projection_pos, projection_pos, projection_pos[3]);

        float x = MIN(w - 1, (uint32_t)((projection_pos[0] + 1) * 0.5 * w)); 
        float y = MIN(h - 1, (uint32_t)((1 - (projection_pos[1] + 1) * 0.5) * h));

        pixelRGBA(r, x, y, world_vertex->colour.r, world_vertex->colour.g, world_vertex->colour.b, 255);
    }
}
