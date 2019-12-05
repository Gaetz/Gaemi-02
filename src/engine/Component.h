//
// Created by gaetz on 05/12/2019.
//

#ifndef COMPONENT_H
#define COMPONENT_H

#include "Types.h"

class Actor;

class Component {
public:
    /**
     *  Constructor
     * @param owner Actor owning the component
     * @param updateOrder The lower the order, the earlier the component updates
     */
    Component(Actor* owner, int updateOrder = 100);
    virtual ~Component();

    virtual void update(u32 dt);
    u32 getUpdateOrder() const;

private:
    Actor* owner;
    u32 updateOrder;

    Component() {};
};


#endif //COMPONENT_H
