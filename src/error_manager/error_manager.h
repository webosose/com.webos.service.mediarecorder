#pragma once

#include "error.h"
#include <string>
#include <vector>

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

    std::vector<Error> errors;
};
