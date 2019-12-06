//
// Created by gaetz on 05/12/2019.
//

#ifndef ACTOR_H
#define ACTOR_H

#include "Component.h"
#include "MathCore.h"
#include "Types.h"
#include <vector>

class Scene;

using std::vector;

/**
 * Top level game object class.
 *
 * An actor possesses components AND a proper update logic.
 */
class Actor {
public:

    enum State {
        Active, Paused, Expired
    };

    Actor(Scene *scene);

    virtual ~Actor();

    /**
     * Input handling called from scene (not overridable)
     * @param inputState State of the input devices
     */
    void processInput(const InputState &inputState);

    /**
     * Specific Input handling code
     * @param inputState State of the input devices
     */
    virtual void actorInput(const InputState &inputState);

    /**
     * Update function called from scene (not overridable)
     * @param dt Delta time in milliseconds
     */
    void update(u32 dt);

    /**
     * Update actor's components (not overridable)
     * @param dt Delta time in milliseconds
     */
    void updateComponents(u32 dt);

    /**
     * Actor specific update code
     * @param dt Delta time in milliseconds
     */
    virtual void updateActor(u32 dt);

    /**
     * Add a component to the actor
     * @param component Component to be added
     */
    void addComponent(Component *component);

    /**
     * Remove a component from the actor
     * @param component Component to be removed
     */
    void removeComponent(Component *component);

    /**
     * Create a world transform matrix from position, rotation and scale
     * of the actor.
     */
    void computeWorldTransform();

    inline Scene* getScene() const { return scene; }

    State getState() const;

    void setState(State state);

    const Matrix4 &getWorldTransform() const;

    const Vector2 &getPosition() const;

    void setPosition(const Vector2 &position);

    float getRotation() const;

    void setRotation(float rotation);

    float getScale() const;

    void setScale(float scale);


protected:
    /**
     * Actor's state
     */
    State state;

    /**
     * Screen position
     */
    Vector2 position;

    /**
     * Rotation in radians
     */
    f32 rotation;

    /**
     * Uniform scale (1.0f is 100%)
     */
    f32 scale;

    /**
     * Actor's transform
     */
    Matrix4 worldTransform;

    /**
     * True when the position/rotation/scale changes
     */
    bool shouldRecomputeWorldTransform;

    /**
     * Actor's components
     */
    std::vector<Component *> components;

    /**
     * Scene containing this actor
     */
    Scene *scene;

private:
    Actor();
};


#endif //ACTOR_H
