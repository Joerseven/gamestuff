//
// Created by c3042750 on 05/12/2023.
//

#include "TweenManager.h"

void TweenManager::Create(Transform* character, const Transform& target, float duration) {
    TweenInfo t;
    t.original = Transform(*character); // Copying this boi
    t.current = character;
    t.target = target;
    t.duration = duration;
    t.elapsed = 0.0f;
    tweens[character] = t;
//    if (tweens.find(character) == tweens.end()) {
//        tweens.insert(std::make_pair(character, t));
//    } else {
//        tweens[character] = t;
//    }
}

void TweenManager::Update(float dt) {
    for (auto it = tweens.begin(); it != tweens.end();) {
        auto& v = it->second;
        v.elapsed += dt;
        if (v.elapsed >= v.duration) {
            *(v.current) = v.target;
            tweens.erase(it++);
        } else {
            v.current->SetPosition(Vector3::Lerp(v.original.GetPosition(), v.target.GetPosition(), v.elapsed / v.duration));
            v.current->SetOrientation(Quaternion::Lerp(v.original.GetOrientation(), v.target.GetOrientation(), v.elapsed / v.duration).Normalised());
            it++;
        }
    }
}