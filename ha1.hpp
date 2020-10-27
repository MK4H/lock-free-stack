/*

AdvPara - NPRG058

Home Assignment 1

Lock-free structure

*/

#ifndef NPRG058_HA1_LOCKFREE_GUARD__
#define NPRG058_HA1_LOCKFREE_GUARD__

#include <atomic>
#include <cstdint>

namespace impl {

template <typename T>
struct LinkedListNode;

/*
 * Packed to fit it into the 16B so we can use cmpxchg16b instruction
 * Aligned due to the requirements of the cmpxchg16b
 * WARNING: IS ONLY GUARANTEED TO WORK ON 64bit platforms
*/
template <typename T>
struct LinkedListHandle {
	uint64_t tag;
	LinkedListNode<T> *head;
} __attribute__((packed, aligned(16)));

template <typename T>
struct LinkedListNode {
	T value;
	LinkedListNode *next;

	LinkedListNode(const T &value)
		:value(value), next(nullptr)
	{ }
};

}

template <typename T>
class LFStack {
public:
	void push(const T &v);
	T pop();

	bool empty() const;
private:
	std::atomic<impl::LinkedListHandle<T>> head_handle;
};

template <typename T>
void LFStack<T>::push(const T &v) {

	impl::LinkedListNode<T> *n_head = new impl::LinkedListNode<T>(v);

	for (;;) {
		impl::LinkedListHandle<T> c_handle = head_handle.load(std::memory_order_acquire);
		impl::LinkedListHandle<T> n_handle = c_handle;
		n_head->next = n_handle.head;
		n_handle.head = n_head;
		++n_handle.tag;
		// On success, we need to release all the writes
		// On failure, any reads or writes we did in the cycle do not matter, so relax
		if (head_handle.compare_exchange_weak(c_handle, n_handle, std::memory_order_release, std::memory_order_relaxed)){
			break;
		}
	}
}

template <typename T>
T LFStack<T>::pop() {
	impl::LinkedListNode<T> *popped;

	for (;;) {
		impl::LinkedListHandle<T> c_handle = head_handle.load(std::memory_order_acquire);
		impl::LinkedListHandle<T> n_handle = c_handle;

		popped = n_handle.head;
		n_handle.head = n_handle.head->next;
		++n_handle.tag;
		if (head_handle.compare_exchange_weak(c_handle, n_handle, std::memory_order_release, std::memory_order_relaxed)){
			break;
		}
	}

	T value = std::move(popped->value);
	delete popped;
	return value;
}

template <typename T>
bool LFStack<T>::empty() const {
	// Could probably go with relaxed memory order
	return head_handle.load(std::memory_order_acquire).head == nullptr;
}

#endif // !NPRG058_HA1_LOCKFREE_GUARD__
