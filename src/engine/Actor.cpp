//
// Created by gaetz on 05/12/2019.
//

#include "Actor.h"
#include "Scene.h"
#include <algorithm>

Actor::Actor(Scene *scene)
        : state(Pending), position(Vector2::zero), scale(1.0f), rotation(0.0f), scene(scene),
          shouldRecomputeWorldTransform(true) {
    scene->addActor(this);
}


Actor::~Actor() {
    scene->removeActor(this);
    while (!components.empty()) {
        delete components.back();
    }
}

Actor::State Actor::getState() const {
    return state;
}

void Actor::setState(Actor::State state) {
    this->state = state;
}

const Matrix4 &Actor::getWorldTransform() const { return worldTransform; }

const Vector2 &Actor::getPosition() const {
    return position;
}

void Actor::setPosition(const Vector2 &position) {
    Actor::position = position;
    shouldRecomputeWorldTransform = true;
}

float Actor::getRotation() const {
    return rotation;
}

void Actor::setRotation(float rotation) {
    Actor::rotation = rotation;
    shouldRecomputeWorldTransform = true;
}

float Actor::getScale() const {
    return scale;
}

void Actor::setScale(float scale) {
    Actor::scale = scale;
    shouldRecomputeWorldTransform = true;
}

void Actor::processInput(const InputState &inputState) {
    if(state == Active) {
        for (auto component : components) {
            component->processInput(inputState);
        }
        actorInput(inputState);
    }
}

void Actor::actorInput(const InputState &inputState) {

}

void Actor::update(u32 dt) {
    if (state == Active) {
        computeWorldTransform();
        updateComponents(dt);
        updateActor(dt);
        computeWorldTransform();
    }
}

void Actor::updateComponents(u32 dt) {
    for (auto comp : components) {
        comp->update(dt);
    }
}

void Actor::updateActor(u32 dt) {
}

void Actor::computeWorldTransform() {
    if (shouldRecomputeWorldTransform) {
        shouldRecomputeWorldTransform = false;
        // Scale, then rotate, then translate
        worldTransform = Matrix4::createScale(scale);
        worldTransform *= Matrix4::createRotationZ(rotation);
        worldTransform *= Matrix4::createTranslation(Vector3(position.x, position.y, 0.0f));

        // Inform components world transform updated
        for (auto comp : components) {
            comp->onUpdateWorldTransform();
        }
    }
}

void Actor::addComponent(Component *component) {
    i32 order = component->getUpdateOrder();
    // Find the first element with order higher than added component
    auto iter = components.begin();
    for(;iter != components.end(); ++iter) {
        if(order < (*iter)->getUpdateOrder()) {
            break;
        }
    }
    // Insert element before position of iterator
    components.insert(iter, component);
}

void Actor::removeComponent(Component *component) {
    auto iter = std::find(components.begin(), components.end(), component);
    if(iter != components.end()) {
        components.erase(iter);
    }
}

