#pragma once

#include <stdexcept>


namespace TwoPartyMaskedEvaluation
{
	class NoInputAddedException : public std::exception
	{
	};

	class NotEvaluatedException : public std::exception
	{
	};

	class AlreadyEvaluatedException : public std::exception
	{
	};

	class BeaverSharesNotFound : public std::exception
	{
	};
	
	class ErrorExceedsBound : public std::exception
	{
	};
}
