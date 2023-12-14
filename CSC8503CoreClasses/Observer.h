//
// Created by c3042750 on 11/12/2023.
//

#ifndef CSC8503_OBSERVER_H
#define CSC8503_OBSERVER_H

template <typename ...Args>
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual bool Receive(const Args&... eventData) = 0;
};

template <typename ...Args>
class ISubject {
public:
    virtual ~ISubject() = default;
    virtual void Attach(IObserver<Args...> *observer) = 0;
    virtual void Detach(IObserver<Args...> *observer) = 0;
    virtual void Trigger(const Args&... data) = 0;
};

template <typename ...Args>
class Subject: public ISubject<Args...> {
public:

    virtual~ Subject() {}

    void Attach(IObserver<Args...> *observer) override {
        observers.push_back(observer);
    }

    void Detach(IObserver<Args...> *observer) override {
        std::cout << "Started at: " << observers.size() << std::endl;
        observers.remove(observer);
        std::cout << "Removed at: " << observers.size() << std::endl;

    }

    void Trigger(const Args&... data) override {
        for (auto it = observers.begin(); it != observers.end();) {
            if ((*it)->Receive(data...)) {
                observers.erase(it++);
                std::cout << "Erasing self, observers now at: " << observers.size() << std::endl;
            } else {
                ++it;
            }
        }
    }

private:
    std::list<IObserver<Args...>*> observers;
};

template <typename ...Args>
class Observer : public IObserver<Args...> {
public:

    Observer() = default;

    Observer(std::function<bool(Args...)> triggerFunction) {
        onTrigger = triggerFunction;
    }

    void SetOnTrigger(std::function<bool(Args...)> triggerFunction) {
        onTrigger = triggerFunction;
    }

    bool Receive(const Args&... args) override {
        return onTrigger(args...);
    }

private:
    std::function<bool(Args...)> onTrigger;
};


#endif //CSC8503_OBSERVER_H
