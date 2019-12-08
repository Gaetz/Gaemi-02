//
// Created by gaetz on 05/12/2019.
//

#include "TestActor.h"

#include "../engine/SpriteComponent.h"
#include "../engine/ResourceManager.h"
#include "SceneMain.h"

TestActor::TestActor(Scene *scene)
        : Actor(scene) {
    SpriteComponent *sc = new SpriteComponent(this, 100);
    sc->setTexture(ResourceManager::getTexture("tile"));
    addComponent(sc);
    position = Vector2(100, 100);
}

TestActor::~TestActor() {}
