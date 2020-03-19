#ifndef _TIMER_H__
#define _TIMER_H__

#include <sys/time.h>
#include <sstream>
#include <iostream>
#include <stdio.h>

class Timer 
{
public:
	struct timeval diff, startTV, endTV;
	
	Timer()
	{
		gettimeofday(&startTV, NULL); 
	}
	
	void Tick(std::string str)
	{
		gettimeofday(&endTV, NULL); 
		timersub(&endTV, &startTV, &diff);
		
		printf("**%s time: %f seconds\n", str.c_str(), diff.tv_sec + (float)diff.tv_usec/1000000.0);
		
		startTV = endTV;
	}
};

/* class Timer 
{
private:
	clock_t begin;
	clock_t end;
public:
	Timer()
	{
		begin = clock();
	}
	
	~Timer(){}
	
	void Reset()
	{
		begin = clock();
	}
	
	void Tick(const std::string& str)
	{
		end = clock();
		std::stringstream ss;
		ss << str << ": ";
		ss << (double)(end - begin)/CLOCKS_PER_SEC << " seconds\n";
		std::cout << ss.str() << std::endl;
		begin = clock();
	}
}; */

#endif
