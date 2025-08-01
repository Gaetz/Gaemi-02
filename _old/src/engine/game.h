#ifndef GAME_H
#define GAME_H

#ifdef __linux__
	#include <SDL2/SDL.h>
#elif _WIN32
	#include <SDL.h>
#endif

#include <GL/glew.h>
#include <vector>
#include <memory>

#include "InputManager.h"
#include "Shader.h"
#include "Types.h"

class Scene;

// This game class manages game states and triggers their logic.
// It supports gamestate stacking. It does not implement a 
// gameobject/entity/whatever logic to let you free to choose
// your architecture.
class game
{
public:
	game();
	virtual ~game();

	void init(u16 screenWidth, u16 screenHeight);
	void load();
	void handleInputs();
	void update(u32 dt);
	void render();
	void clean();

	void changeState(std::unique_ptr<Scene>);
	void pushState(std::unique_ptr<Scene>);
	void popState();

	bool isRunning;
	int windowWidth, windowHeight;

private:
    std::unique_ptr<InputManager> inputManager;
	std::vector<std::unique_ptr<Scene>> gameStates;
};

#endif