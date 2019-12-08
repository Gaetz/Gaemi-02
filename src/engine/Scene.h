#ifndef SCENE_H
#define SCENE_H

#include "Game.h"
#include "InputState.h"
#include "SpriteComponent.h"

class Actor;

// Interface for scenes
class Scene {
public:
	virtual ~Scene() {}

	virtual void load() = 0;
	virtual void clean() = 0;
	
	virtual void handleEvent(const InputState&) = 0;
	virtual void update(unsigned int dt) = 0;
	virtual void draw() = 0;

	virtual void pause() = 0;
	virtual void resume() = 0;

    virtual void addActor(Actor* actor) = 0;
    virtual void removeActor(Actor* actor) = 0;

	virtual void setGame(Game *_game) = 0;

    virtual void addSprite(SpriteComponent* sprite) = 0;
    virtual void removeSprite(SpriteComponent* sprite) = 0;
};

#endif