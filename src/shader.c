#include <GL/glew.h>
#include <cglm/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"
#include "shader.h"

static GLchar *read_shader_source(const char *path);
static GLuint compile_shader(GLenum shader_type, const GLchar *shader_source);
static GLint check_shader_compile_errors(GLuint shader);
static GLint check_shader_program_link_errors(GLuint program);
static uint64_t shader_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int shader_compare(const void *a, const void *b, void *udata);

static uint64_t shader_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const Shader *shader = item;
	return hashmap_sip(shader->name, strlen(shader->name), seed0, seed1);
}

static int shader_compare(const void *a, const void *b, void *udata)
{
	const Shader *ua = a;
	const Shader *ub = b;
	return strcmp(ua->name, ub->name);
}

ShaderProgram create_shader(const char *vertex_shader_path,
			    const char *fragment_shader_path)
{
	GLchar *vertex_source = read_shader_source(vertex_shader_path);
	if (vertex_source == NULL) {
		return 0;
	}

	GLchar *fragment_source = read_shader_source(fragment_shader_path);
	if (fragment_source == NULL) {
		return 0;
	}

	GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
	free(vertex_source);
	if (!vertex_shader) {
		return 0;
	}

	GLuint fragment_shader =
	    compile_shader(GL_FRAGMENT_SHADER, fragment_source);
	free(fragment_source);
	if (!fragment_shader) {
		return 0;
	}

	GLuint shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	if (!check_shader_program_link_errors(shader_program)) {
		return 0;
	}

	return shader_program;
}

static GLuint compile_shader(GLenum shader_type, const GLchar *shader_source)
{
	GLuint shader = glCreateShader(shader_type);
	glShaderSource(shader, 1, &shader_source, NULL);
	glCompileShader(shader);
	if (!check_shader_compile_errors(shader)) {
		return 0;
	}

	return shader;
}

static GLint check_shader_compile_errors(GLuint shader)
{
	GLint success;
	GLchar log[512];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, log);
		printf("ERROR::SHADER::COMPILATION_FAILED\n%s\n", log);
	}

	return success;
}

static GLint check_shader_program_link_errors(GLuint program)
{
	GLint success;
	GLchar log[512];

	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, log);
		printf("ERROR::SHADER::LINKING_FAILED\n%s\n", log);
	}

	return success;
}

void shader_program_set_texture_samplers(ShaderProgram program,
					 const GLint *samplers,
					 const GLint sampler_count)
{
	glUniform1iv(glGetUniformLocation(program, "u_Textures"), sampler_count,
		     samplers);
}

void shader_program_set_texture(ShaderProgram program, const GLint texture)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(program, "u_Texture"), 0);
}

void preload_shaders(ShaderCache *cache)
{
	shader_cache_put(
	    cache, "scene",
	    create_shader("src/shaders/scene.vs", "src/shaders/scene.fs"));

	shader_cache_put(
	    cache, "text",
	    create_shader("src/shaders/text.vs", "src/shaders/text.fs"));
}

int shader_cache_init(ShaderCache **cache)
{
	*cache = hashmap_new(sizeof(Shader), 0, 0, 0, shader_hash,
			     shader_compare, NULL);
	if (*cache == NULL) {
		printf("Failed to allocate memory for shader cache.\n");
		return 0;
	}
	return 1;
}

ShaderProgram shader_cache_get(ShaderCache *cache, char *shader_name)
{
	Shader *cached_shader =
	    hashmap_get(cache, &(Shader){.name = shader_name});
	if (cached_shader == NULL) {
		printf("Failed to load cached shader.\n");
		return 0;
	}
	return cached_shader->id;
}

void shader_cache_put(ShaderCache *cache, char *shader_name,
		      ShaderProgram shader_id)
{
	hashmap_set(cache, &(Shader){.name = shader_name, .id = shader_id});
}

static GLchar *read_shader_source(const char *path)
{
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		printf("could not load file at %s\n", path);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	GLchar *buf = malloc(sizeof(GLchar) * (length + 1));
	fread(buf, 1, length, file);
	buf[length] = '\0';

	fclose(file);

	return buf;
}
