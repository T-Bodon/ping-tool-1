#ifndef PING_PING_ERROR_H
#define PING_PING_ERROR_H

#include <string>
#include <stdexcept>

class ping_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
    using std::runtime_error::what;
};

#endif //PING_PING_ERROR_H
