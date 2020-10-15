/*

AdvPara - NPRG058

Home Assignment 1

Lock-free structure

*/

#ifndef NPRG058_HA1_LOCKFREE_GUARD__
#define NPRG058_HA1_LOCKFREE_GUARD__

#include <atomic>

template <typename T>
class LFStack {
public:
	void push(const T &v);
	T pop();

	bool empty() const;
private:
};

#endif // !NPRG058_HA1_LOCKFREE_GUARD__
