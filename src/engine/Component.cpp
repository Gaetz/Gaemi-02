//
// Created by gaetz on 05/12/2019.
//

#include "Component.h"
#include "Actor.h"

Component::Component(Actor *owner, i32 updateOrder) {
    owner->addComponent(this);
}

Component::~Component() {
    owner->removeComponent(this);
}

