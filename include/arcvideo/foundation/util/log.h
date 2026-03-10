#ifndef ARCVIDEO_FOUNDATION_LOG_H
#define ARCVIDEO_FOUNDATION_LOG_H

#include <iostream>

namespace arcvideo::foundation {

class Log {
public:
    Log(const char* type) { std::cerr << "[" << type << "]"; }

    ~Log() { std::cerr << std::endl; }

    template <typename T>
    Log& operator<<(const T& t) {
        std::cerr << " " << t;
        return *this;
    }

    static Log Debug() { return {"DEBUG"}; }

    static Log Info() { return {"INFO"}; }

    static Log Warning() { return {"WARNING"}; }

    static Log Error() { return {"ERROR"}; }
};

}  // namespace arcvideo::foundation

#endif  // ARCVIDEO_FOUNDATION_LOG_H
