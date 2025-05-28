#ifndef GAMESTATE_MAIN_H
#define GAMESTATE_MAIN_H

#include "../engine/Scene.h"
#include "../engine/Actor.h"
#include "../engine/VertexArray.h"
#include "TestActor.h"

#include <vector>

static std::array<GLfloat, 18> vertexBuffer = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,

        0.5f, 0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f
};

static std::array<GLfloat, 12> texBuffer = {
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f
};

// Main scene of the game, contains all the game's logic
class SceneMain : public Scene
{
public:
	SceneMain();
	virtual ~SceneMain();

	void load();
	void clean();
	void pause();
	void resume();
	void handleEvent(const InputState &);
	void update(unsigned int dt);
	void draw();
	void setGame(game *);

	/**
	 * Add an actor into the scene
	 * @param actor Actor to be added
	 */
    void addActor(Actor* actor);

    /**
     * Remove an actor from the scene if it is found
     * @param actor Actor to be removed
     */
    void removeActor(Actor* actor);

    /**
     * Remove all actors with state Expired
     */
    void removeExpiredActors();

    void addSprite(SpriteComponent* sprite);
    void removeSprite(SpriteComponent* sprite);

private:
    TestActor* testActor;
    /**
     * Scene's actors
     */
	std::vector<Actor*> actors;

	/**
	 * Actors waiting to be added to the scene
	 */
    std::vector<Actor*> pendingActors;

    /**
     * True when actors' collection is locked
     * for calling update function
     */
    bool isUpdatingActors;

    VertexArray vertexArray;
    std::vector<SpriteComponent*> sprites;

    game *game;
    int getRand(i32 a, i32 b);
};

#endif
