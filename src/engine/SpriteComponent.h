//
// Created by gaetz on 05/12/2019.
//

#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H


#include "Component.h"
#include "Texture2D.h"
#include "Shader.h"

class Actor;

class SpriteComponent : public Component {
public:
    SpriteComponent(Actor *owner, i32 drawOrder = 100);

    virtual ~SpriteComponent();

    virtual void draw(Shader* shader);

    virtual void setTexture(Texture2D *texture);

    inline i32 getDrawOrder() const {
        return drawOrder;
    }

private:
    Texture2D *texture;
    i32 drawOrder;
};


#endif // SPRITECOMPONENT_H
