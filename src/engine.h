#pragma once
#include "menu.h"
#include "minigame.h"
#include "resource_cache.h"
#include "scene.h"
#include "shader.h"
#include "state.h"
#include "texture.h"
#include "kumashoot.h"

typedef struct {
	ResourceCache *resource_cache;
	GLFWwindow *main_window;
	Menu *menu;
	GLFWwindow *minigame_window;
	Minigame *minigame;
	GameState *game_state;
} Engine;

int engine_init(Engine *engine);
void engine_run(Engine *engine);
