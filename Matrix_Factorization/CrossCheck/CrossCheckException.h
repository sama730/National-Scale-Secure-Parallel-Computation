#ifndef CROSSCHECK_EXCEPTION_H__
#define CROSSCHECK_EXCEPTION_H__

#pragma once
#include <string>
#include <stdexcept>


namespace CrossCheck
{
	class NoPreprocessingGeneratedException : public std::exception
	{
	public:
		NoPreprocessingGeneratedException() : std::exception() {}
	};

	class NotCrossCheckedException : public std::exception
	{
	public:
		NotCrossCheckedException() : std::exception() {}
	};

	class InconsistentInputException : public std::exception
	{
	public:
		InconsistentInputException(const std::string &str) : std::exception() {}
	};

	class InconsistentSeedException : public std::exception
	{
	public:
		InconsistentSeedException(const std::string &str) : std::exception() {}
	};

	class InconsistentHashException : public std::exception
	{
	public:
		InconsistentHashException(const std::string &str) : std::exception() {}
	};

	class VetoException : public std::exception
	{
	public:
		VetoException() : std::exception() {}
	};
}

#endif
