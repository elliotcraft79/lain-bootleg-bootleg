#include <cglm/cglm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource_cache.h"
#include "scene.h"
#include "sprite.h"
#include "text.h"

const static uint16_t white_glyphs_order[256] = {
    ['A'] = 0,	['P'] = 1,  ['0'] = 2, ['1'] = 3, ['2'] = 4,
    ['3'] = 5,	['4'] = 6,  ['5'] = 7, ['6'] = 8, ['7'] = 9,
    ['8'] = 10, ['9'] = 11, [':'] = 12};

static void init_text_obj_buffers(Text *text_obj)
{
	glGenVertexArrays(1, &text_obj->VAO);
	glBindVertexArray(text_obj->VAO);

	glGenBuffers(1, &text_obj->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, text_obj->VBO);

	glGenBuffers(1, &text_obj->IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, text_obj->IBO);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			      (void *)0);
	// texture coords
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			      (void *)(2 * sizeof(GLfloat)));
}

int init_text_obj(Text **text_obj, Texture *texture,
		  ResourceCache *resource_cache)
{
	*text_obj = malloc(sizeof(Text));
	if (*text_obj == NULL) {
		printf("Failed to allocate memory for text renderer.\n");
		return 0;
	}

	(*text_obj)->current_text = malloc(sizeof(char) * 10);
	if ((*text_obj)->current_text == NULL) {
		printf("Failed to allocate memory for text inside the text "
		       "sprite.\n");
		return 0;
	}

	init_text_obj_buffers(*text_obj);

	(*text_obj)->shader = resource_cache->shaders[TEXT_SHADER];
	(*text_obj)->texture = texture;

	return 1;
}

_Bool text_obj_needs_update(Text *text_obj, unsigned char *text)
{
	return (strcmp((char *)text_obj->current_text, (char *)text) != 0);
}

void update_text(Text *text_obj, unsigned char *text, unsigned int sprite_count)
{
	GLfloat vertices[get_sprite_vertex_buffer_size(sprite_count)];
	GLfloat *buffer_ptr = vertices;

	for (int i = 0; i < strlen((char *)text); i++) {
		sprite_count++;

		const Vector2D pos = {text_obj->pos.x + i * text_obj->h_padding,
				      text_obj->pos.y};

		Sprite glyph =
		    (Sprite){.pos = pos,
			     .size = text_obj->glyph_size,
			     .texture_size = text_obj->glyph_texture_size,
			     .current_frame = white_glyphs_order[text[i]]};

		buffer_ptr = get_sprite_vertices(buffer_ptr, &glyph);

		// AM and PM count as a single character.
		if (text[i] == 'A' || text[i] == 'P') {
			i++;
		}
	}

	glBindVertexArray(text_obj->VAO);

	update_sprite_buffers(text_obj->VBO, text_obj->IBO, vertices,
			      sizeof(vertices), sprite_count);

	text_obj->sprite_count = sprite_count;
}

void draw_text(Text *text_obj, GLFWwindow *window)
{
	// window data
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);

	// bind shader and set texture
	glUseProgram(text_obj->shader);
	shader_program_set_texture(text_obj->shader, text_obj->texture->id);

	// set up matrices
	mat4 proj, model, view;

	glm_ortho(-w / 2.0f, w / 2.0f, -h / 2.0f, h / 2.0f, -1.0f, 1.0f, proj);

	glm_mat4_identity(model);
	glm_mat4_identity(view);

	shader_program_set_mat4(text_obj->shader, "u_Projection", proj);
	shader_program_set_mat4(text_obj->shader, "u_Model", model);
	shader_program_set_mat4(text_obj->shader, "u_View", view);

	// bind vao
	glBindVertexArray(text_obj->VAO);

	// draw
	glDrawElements(GL_TRIANGLES,
		       get_sprite_index_count(text_obj->sprite_count),
		       GL_UNSIGNED_INT, 0);
}
