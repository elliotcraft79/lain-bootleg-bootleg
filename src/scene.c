#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CVECTOR_LOGARITHMIC_GROWTH

#include "scene.h"
#include "shader.h"
#include "sprite.h"

#define MAX_SCENE_SPRITES 50
#define MAX_SCENE_TEXTURES 20

#define MAX_SCENE_VBO_SIZE MAX_SCENE_SPRITES *SPRITE_VBO_SIZE
#define MAX_SCENE_IBO_SIZE MAX_SCENE_SPRITES *SPRITE_INDEX_COUNT

// clang-format off
#define IDENTITY_MAT4   {{1.0f, 0.0f, 0.0f, 0.0f},                   \
			{0.0f, 1.0f, 0.0f, 0.0f},                    \
			{0.0f, 0.0f, 1.0f, 0.0f},                    \
			{0.0f, 0.0f, 0.0f, 1.0f}}

#define ZEROS_MAT4      {{0.0f, 0.0f, 0.0f, 0.0f},                   \
			{0.0f, 0.0f, 0.0f, 0.0f},                    \
			{0.0f, 0.0f, 0.0f, 0.0f},                    \
			{0.0f, 0.0f, 0.0f, 0.0f}}
// clang-format on

static GLfloat *get_click_barrier_vertices_vertices(GLfloat *buffer,
						    ClickBarrier *barrier)
{
	GLfloat vertices[] = {
	    // top right
	    barrier->right,
	    barrier->top,
	    0.0f,
	    0.0f,
	    -1.0f,

	    // bottom right
	    barrier->right,
	    barrier->bottom,
	    0.0f,
	    0.0f,
	    -1.0f,

	    // bottom left
	    barrier->left,
	    barrier->bottom,
	    0.0f,
	    0.0f,
	    -1.0f,

	    // top left
	    barrier->left,
	    barrier->top,
	    0.0f,
	    0.0f,
	    -1.0f,
	};

	memcpy(buffer, vertices, sizeof(vertices));
	buffer += sizeof(vertices) / sizeof(vertices[0]);
	return buffer;
}

static void init_scene_buffers(Scene *scene)
{
	glGenVertexArrays(1, &scene->VAO);
	glBindVertexArray(scene->VAO);

	glGenBuffers(1, &scene->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, scene->VBO);

	glGenBuffers(1, &scene->IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scene->IBO);

	GLuint index_buffer[MAX_SCENE_IBO_SIZE];
	GLuint *p = index_buffer;

	unsigned int offset = 0;
	for (int i = 0; i < MAX_SCENE_IBO_SIZE; i += 6) {
		GLuint indices[] = {
		    0 + offset, 1 + offset, 3 + offset,
		    1 + offset, 2 + offset, 3 + offset,

		};

		memcpy(p, indices, sizeof(indices));
		p += sizeof(indices) / sizeof(GLuint);

		offset += 4;
	};

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_buffer),
		     index_buffer, GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			      (void *)0);
	// texture coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			      (void *)(2 * sizeof(GLfloat)));

	// texture id
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			      (void *)(4 * sizeof(GLfloat)));
}

void init_scene(Scene *scene, Sprite **sprites, uint8_t sprite_count,
		SpriteBehavior *sprite_behaviors, uint8_t sprite_behavior_count,
		Text **text_objs, uint8_t text_obj_count,
		ClickBarrier *click_barriers, uint8_t click_barrier_count)
{
	// initialize vao, vbo, ibo
	init_scene_buffers(scene);

	scene->sprites = NULL;
	depth_sort(sprites, sprite_count);
	for (int i = 0; i < sprite_count; i++) {
		Sprite *sprite = sprites[i];

		cvector_push_back(scene->sprites, sprite);
	}

	scene->sprite_behaviors = NULL;
	for (int i = 0; i < sprite_behavior_count; i++) {
		cvector_push_back(scene->sprite_behaviors, sprite_behaviors[i]);
	}

	scene->text_objects = NULL;
	for (int i = 0; i < text_obj_count; i++) {
		Text *text = text_objs[i];

		text->origin_pos = text->pos;

		cvector_push_back(scene->text_objects, text);
	}

	scene->click_barriers = NULL;
	for (int i = 0; i < click_barrier_count; i++) {
		cvector_push_back(scene->click_barriers, click_barriers[i]);
	}

	scene->draw_barriers = false;

	update_scene(scene);
}

void update_scene(Scene *scene)
{
	glBindVertexArray(scene->VAO);

	GLfloat vertices[MAX_SCENE_VBO_SIZE];
	GLfloat *buffer_ptr = vertices;

	uint8_t quad_count = 0;

	for (int i = 0; i < cvector_size(scene->sprites); i++) {
		Sprite *sprite = scene->sprites[i];

		sprite->texture_index = i;

		if (!sprite->visible) {
			continue;
		}

		if (sprite->pivot_centered) {
			buffer_ptr = get_pivot_centered_sprite_vertices(
			    buffer_ptr, sprite);
		} else {
			buffer_ptr = get_sprite_vertices(buffer_ptr, sprite);
		}

		quad_count++;
	};

	for (int i = 0; i < cvector_size(scene->text_objects); i++) {
		Text *text_obj = scene->text_objects[i];

		text_obj->texture_index = i + cvector_size(scene->sprites);

		if (!text_obj->visible) {
			continue;
		}

		char *text = text_obj->current_text;
		uint8_t text_len = strlen(text);

		for (int j = 0; j < text_len; j++) {
			buffer_ptr = get_glyph_vertices(buffer_ptr, text_obj,
							text[j], j);
			quad_count++;

			if (text[j] == 'A' || text[j] == 'P') {
				j++;
			}
		}
	}

	if (scene->draw_barriers) {
		for (int i = 0; i < cvector_size(scene->click_barriers); i++) {
			buffer_ptr = get_click_barrier_vertices_vertices(
			    buffer_ptr, &scene->click_barriers[i]);

			quad_count++;
		}
	}

	scene->quad_count = quad_count;

	glBindBuffer(GL_ARRAY_BUFFER, scene->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
		     GL_STREAM_DRAW);
}

void draw_scene(Scene *scene, GLFWwindow *window, ShaderProgram shader)
{
	// window data
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);

	// bind appropriate textures
	for (int i = 0; i < cvector_size(scene->sprites); i++) {
		Sprite *curr = scene->sprites[i];
		if (curr->visible) {
			glActiveTexture(GL_TEXTURE0 + curr->texture_index);
			glBindTexture(GL_TEXTURE_2D, curr->texture->gl_id);
		}
	}

	for (int i = 0; i < cvector_size(scene->text_objects); i++) {
		Text *curr = scene->text_objects[i];
		glActiveTexture(GL_TEXTURE0 + curr->texture_index);
		glBindTexture(GL_TEXTURE_2D, curr->font->texture->gl_id);
	}

	// bind shader and set texture samplers
	glUseProgram(shader);
	GLint samplers[MAX_SCENE_TEXTURES];

	uint8_t texture_count =
	    cvector_size(scene->text_objects) + cvector_size(scene->sprites);

	for (int i = 0; i < texture_count; i++) {
		samplers[i] = i;
	}

	shader_program_set_texture_samplers(shader, samplers, texture_count);

	// set up matrices

	const GLfloat view[4][4] = IDENTITY_MAT4;
	const GLfloat model[4][4] = IDENTITY_MAT4;

	GLfloat proj[4][4] = ZEROS_MAT4;
	float rl = 1.0f / (w - 0.0f);
	float tb = 1.0f / (0.0f - h);
	float fn = -1.0f / (1.0f - -1.0f);

	proj[0][0] = 2.0f * rl;
	proj[1][1] = 2.0f * tb;
	proj[2][2] = 2.0f * fn;
	proj[3][0] = -(w + 0.0f) * rl;
	proj[3][1] = -(0.0f + h) * tb;
	proj[3][2] = (1.0f + -1.0f) * fn;
	proj[3][3] = 1.0f;

	shader_program_set_mat4(shader, "u_Projection", proj);
	shader_program_set_mat4(shader, "u_Model", model);
	shader_program_set_mat4(shader, "u_View", view);

	// bind vao
	glBindVertexArray(scene->VAO);

	// draw
	glDrawElements(GL_TRIANGLES, scene->quad_count * SPRITE_INDEX_COUNT,
		       GL_UNSIGNED_INT, 0);
}

void free_scene(Scene *scene)
{
	cvector_free(scene->sprites);
	cvector_free(scene->click_barriers);
	cvector_free(scene->text_objects);
	cvector_free(scene->sprite_behaviors);
}
