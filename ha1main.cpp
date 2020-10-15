/*

AdvPara - NPRG058

Home Assignment 1

Lock-free structure

*/

#include <thread>
#include <string>
#include <iostream>
#include <random>
#include <vector>
#include <future>

#include "ha1.hpp"

namespace {

	LFStack<int>	ffs;

	void thr_fnc(std::promise<int> && prom, int n, int natt)
	{
		std::mt19937 eng(n);
		std::uniform_int_distribution<int> rint(0, 1000);
		std::uniform_int_distribution<int> rbool(0, 1);

		int my_depth = 0;
		int sum = 0;

		for(int i=0;i<natt;++i)
		{
			auto updown =  rbool(eng);
			if((updown && my_depth>0) || (my_depth==natt-i))	// try to pop or force to pop
			{
				auto v = ffs.pop();
				sum += v;
				--my_depth;
			}
			else
			{
				auto rn = rint(eng);
				ffs.push(rn);
				++my_depth;
			}
		}

		prom.set_value(sum);
	}
}

int main(int argc, char **argv)
{
	if(argc!=3)
	{
		std::cerr << "Usage: ha1 <nthreads> <nattempts>" << std::endl;
		return 4;
	}

	auto nthr = std::stoi(argv[1]);
	auto natt = std::stoi(argv[2]);

	if(nthr<=0 || natt <=0)
	{
		std::cerr << "Parameters must be positive numbers" << std::endl;
		return 4;
	}

	// launch threads
	std::vector<std::thread> thrs;
	std::vector<std::future<int>> ftrs;

	for(int i=0;i<nthr;++i)
	{
		std::promise<int> prom;
		ftrs.push_back(prom.get_future());

		thrs.push_back(std::thread(thr_fnc, std::move(prom), i, natt));
	}

	// wait for threads
	int tsum = 0;
	for(int i=0;i<nthr;++i)
	{
		tsum += ftrs[i].get();
		thrs[i].join();
	}

	// check sanity
	if(!ffs.empty())
		std::cerr << "Huh, something is inside the stack" << std::endl;

	return 0;
}
