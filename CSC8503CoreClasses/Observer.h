//
// Created by c3042750 on 11/12/2023.
//

#ifndef CSC8503_OBSERVER_H
#define CSC8503_OBSERVER_H

template <typename T>
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void Receive(const T& eventData) = 0;
};

template <typename T>
class ISubject {
public:
    virtual ~ISubject() = default;
    virtual void Attach(IObserver<T> *observer) = 0;
    virtual void Detach(IObserver<T> *observer) = 0;
    virtual void Trigger(const T& data) = 0;
};

template <typename T>
class Subject: public ISubject<T> {
public:

    virtual~ Subject() {}

    void Attach(IObserver<T> *observer) override {
        observers.push_back(observer);
    }

    void Detach(IObserver<T> *observer) override {
        observers.remove(observer);
    }

    void Trigger(const T& data) override {
        for (auto it = observers.begin(); it != observers.end(); it++) {
            (*it)->Receive(data);
        }
    }

private:
    std::list<IObserver<T>*> observers;
};

template <typename T>
class Observer : public IObserver<T> {
public:
    Observer(std::function<void(T)> triggerFunction) {
        onTrigger = triggerFunction;
    }

    void Receive(const T& eventData) override {
        onTrigger(eventData);
    }

private:
    std::function<void(T)> onTrigger;
};


#endif //CSC8503_OBSERVER_H
