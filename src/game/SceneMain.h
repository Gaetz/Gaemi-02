#ifndef GAMESTATE_MAIN_H
#define GAMESTATE_MAIN_H

#include "../engine/Scene.h"
#include "../engine/Actor.h"

#include <vector>
#define SPEED 500

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
	void setGame(Game *);

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

private:
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

    Game *game;

    int getRand(i32 a, i32 b);
};

#endif
