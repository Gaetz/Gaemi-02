//
// Created by gaetz on 05/12/2019.
//

#include "Actor.h"

Actor::State Actor::getState() const {
    return state;
}

void Actor::setState(Actor::State state) {
    Actor::state = state;
}

const Vector2 &Actor::getPosition() const {
    return position;
}

void Actor::setPosition(const Vector2 &position) {
    Actor::position = position;
}

float Actor::getRotation() const {
    return rotation;
}

void Actor::setRotation(float rotation) {
    Actor::rotation = rotation;
}

float Actor::getScale() const {
    return scale;
}

void Actor::setScale(float scale) {
    Actor::scale = scale;
}

void Actor::update(unsigned int dt) {

}
