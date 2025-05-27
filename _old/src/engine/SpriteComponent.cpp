//
// Created by gaetz on 05/12/2019.
//

#include "SpriteComponent.h"
#include "ResourceManager.h"
#include "Actor.h"
#include "Scene.h"
#include "Color.h"

SpriteComponent::SpriteComponent(Actor *owner, i32 drawOrder)
        : Component(owner), texture(nullptr), drawOrder(drawOrder) {
    owner->getScene()->addSprite(this);
}

SpriteComponent::~SpriteComponent() {

}

void SpriteComponent::setTexture(Texture2D *texture) {
    this->texture = texture;
}

void SpriteComponent::draw(Shader *shader) {
    if(texture) {
        Matrix4 scaleMatrix = Matrix4::createScale(texture->width, texture->height, 1.0f);
        Matrix4 world = scaleMatrix * owner->getWorldTransform();
        shader->setVector3f("spriteColor", Color::white.toVector3());
        shader->setMatrix4("world", world);
        texture->setActive();
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

