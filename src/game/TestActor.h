//
// Created by gaetz on 05/12/2019.
//

#ifndef TESTACTOR_H
#define TESTACTOR_H

#include "../engine/Actor.h"

class TestActor : public Actor {

public:
    TestActor(Scene *scene);

    virtual ~TestActor();
private:
    TestActor();
};


#endif //TESTACTOR_H
