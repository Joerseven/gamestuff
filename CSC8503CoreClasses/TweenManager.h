//
// Created by c3042750 on 05/12/2023.
//

#ifndef CSC8503_TWEENMANAGER_H
#define CSC8503_TWEENMANAGER_H

#include "Transform.h"

using namespace NCL;
using namespace CSC8503;

class TweenManager {
    struct TweenInfo {
        Transform original;
        Transform target;
        Transform* current;
        float duration;
        float elapsed;
    };
public:
    void Create(Transform* character, const Transform& target, float duration);
    void Update(float dt);
private:
    std::unordered_map<Transform*, TweenInfo> tweens;
};

#endif //CSC8503_TWEENMANAGER_H
