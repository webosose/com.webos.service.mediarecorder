#include "error.h"

Error::Error(int code, const std::string &message) : code(code), message(message) {}

int Error::getCode() const { return code; }

const std::string &Error::getMessage() const { return message; }
