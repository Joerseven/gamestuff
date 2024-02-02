#pragma once
namespace NCL::CSC8503 {
    class  State {

    public:
        typedef std::function<void(float)> StateUpdateFunction;
        State() {}
        State(StateUpdateFunction someFunc) {
            func		= someFunc;
        }
        void Update(float dt)  {
            if (func != nullptr) {
                func(dt);
            }
        }

    protected:
        StateUpdateFunction func;
    };
}

