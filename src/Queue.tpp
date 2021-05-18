#ifndef VIRALINK_QUEUE_TPP
#define VIRALINK_QUEUE_TPP

#include <Arduino.h>

template<class T>
class Queue {
private:
    std::vector<T> list;
    uint16_t size;
public:
    Queue(uint16_t size) {
        this->size = size;
    }

    ~Queue() {
        clear();
    }

    bool isFull();

    bool isEmpty();

    uint16_t getCounts();

    bool push(const T &item);

    T peek();

    T pop();

    void clear();
};

template<class T>
bool Queue<T>::isFull() {
    return list.size() == size;
}

template<class T>
bool Queue<T>::isEmpty() {
    return list.empty();
}

template<class T>
uint16_t Queue<T>::getCounts() {
    return list.size();
}

template<class T>
bool Queue<T>::push(const T &item) {
    if (isFull()) return false;

    list.push_back(item);
    return true;
}

template<class T>
T Queue<T>::pop() {
    if (isEmpty()) return T(); // Returns empty
    T result = list.front();
    list.erase(list.begin());
    return result;
}

template<class T>
T Queue<T>::peek() {
    if (isEmpty()) return T(); // Returns empty
    return list.front();
}

template<class T>
void Queue<T>::clear() {
    list.clear();
}

#endif //VIRALINK_QUEUE_TPP
