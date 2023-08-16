#pragma once

#include "error.h"
#include <map>
#include <string>

class ErrorManager
{
public:
    static ErrorManager &getInstance();
    Error getError(int code) const;

private:
    ErrorManager();
    ErrorManager(const ErrorManager &)            = delete;
    ErrorManager &operator=(const ErrorManager &) = delete;

    void addError(int code, const std::string &message);
    const std::string &getErrorMessage(int code) const;
    void registerErrors();

    std::map<int, std::string> errors;
};
