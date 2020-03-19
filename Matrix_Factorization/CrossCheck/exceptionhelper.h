#ifndef EXCEPTION_HELPER_H__
#define EXCEPTION_HELPER_H__

#pragma once

#include <stdexcept>

class InvalidDecommitmentException : public std::exception
{
private:
    std::string msg;

public:
    InvalidDecommitmentException(const std::string& message = "") : msg(message)
    {
    }

    const char * what() const throw()
    {
        return msg.c_str();
    }
};

class NoInputAddedException : public std::exception
{
private:
    std::string msg;

public:
    NoInputAddedException(const std::string& message = "") : msg(message)
    {
    }

    const char * what() const throw()
    {
        return msg.c_str();
    }
};

class NotEvaluatedException : public std::exception
{
private:
    std::string msg;

public:
    NotEvaluatedException(const std::string& message = "") : msg(message)
    {
    }

    const char * what() const throw()
    {
        return msg.c_str();
    }
};

#endif

