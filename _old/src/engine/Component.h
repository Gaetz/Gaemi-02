//
// Created by gaetz on 05/12/2019.
//

#ifndef COMPONENT_H
#define COMPONENT_H

#include "Types.h"
#include "InputState.h"

class Actor;

class Component {
public:
    /**
     *  Constructor
     * @param owner Actor owning the component
     * @param updateOrder The lower the order, the earlier the component updates
     */
    Component(Actor* owner, i32 updateOrder = 100);
    virtual ~Component();

    /**
     * Handle input events
     * @param inputState Input state
     */
    virtual void processInput(const InputState &inputState) {};

    /**
     * Update component with delta time
     * @param dt Delta time in milliseconds
     */
    virtual void update(u32 dt) {};

    /**
     * Called when world transform is updated
     */
    virtual void onUpdateWorldTransform() { }

    inline i32 getUpdateOrder() const { return updateOrder; }

protected:
    Actor* owner;
public:
    Actor *getOwner() const;

protected:
    u32 updateOrder;

private:
    Component() {};
};

#endif //COMPONENT_H
