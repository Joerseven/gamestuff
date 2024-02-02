//
// Created by c3042750 on 12/12/2023.
//

#ifndef CSC8503_PAUSESCREEN_H
#define CSC8503_PAUSESCREEN_H

#include "PushdownState.h"
#include "PushdownMachine.h"

using namespace NCL;
using namespace CSC8503;

class MenuScreen : public PushdownState{
    PushdownResult OnUpdate(float dt, PushdownState** newState) {

    }
};

class GameScreen : public PushdownState {

};


#endif //CSC8503_PAUSESCREEN_H
