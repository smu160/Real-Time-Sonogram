#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

#include <mutex>
#include <queue>


template<class T>
class SafeQueue {

private:
    std::queue<T> q;
    std::mutex m;

public:

    void push(T elem) {
        std::lock_guard<std::mutex> lock(m);
        q.push(elem);
    }

    bool pop_front(T& elem) {
        std::lock_guard<std::mutex> lock(m);

        if (q.empty()) {
            return false;
        }
        elem = q.front();
        q.pop();

        return true;
    }

};
#endif
