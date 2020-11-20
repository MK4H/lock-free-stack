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
struct alignas(16) LinkedListHandle {
	uint64_t tag;
	LinkedListNode<T> *head;
};

template <typename T>
struct LinkedListNode {
	T value;
	LinkedListNode *next;

	LinkedListNode()
		:value(), next(nullptr)
	{ }

	LinkedListNode(const T &value)
		:value(value), next(nullptr)
	{ }
};

}

template <typename T>
class LFStack {
public:
	LFStack();
	~LFStack();

	void push(const T &v);
	T pop();

	bool empty() const;
private:
	static_assert(sizeof(impl::LinkedListHandle<T>) == 16);
	std::atomic<impl::LinkedListHandle<T>> head_handle;
	std::atomic<impl::LinkedListHandle<T>> pool_handle;

	impl::LinkedListNode<T>* get_new_node();
	void push_to_handle(std::atomic<impl::LinkedListHandle<T>> &handle, impl::LinkedListNode<T> *n_head);
	std::tuple<bool, impl::LinkedListNode<T>*> try_pop(std::atomic<impl::LinkedListHandle<T>> &handle, impl::LinkedListHandle<T> &c_handle);
};

template<typename T>
LFStack<T>::LFStack()
	:head_handle()
{ }

template<typename T>
LFStack<T>::~LFStack() {
	impl::LinkedListNode<T> *head = head_handle.load(std::memory_order_acquire).head;
	while (head != nullptr) {
		auto to_delete = head;
		head = head->next;
		delete to_delete;
	}

	head = pool_handle.load(std::memory_order_acquire).head;
	while (head != nullptr) {
		auto to_delete = head;
		head = head->next;
		delete to_delete;
	}
}

template <typename T>
void LFStack<T>::push(const T &v) {

	impl::LinkedListNode<T> *n_head = get_new_node();
	n_head->value = v;

	push_to_handle(head_handle, n_head);
}

template <typename T>
T LFStack<T>::pop() {
	impl::LinkedListNode<T> *popped;

	impl::LinkedListHandle<T> c_handle = head_handle.load(std::memory_order_acquire);
	for (;;) {
		auto [success, n_popped] = try_pop(head_handle, c_handle);

		if (success) {
			popped = n_popped;
			break;
		}
	}

	if (popped == nullptr) {
		throw new std::runtime_error{"Popping from empty stack"};
	}

	T value = std::move(popped->value);
	push_to_handle(pool_handle, popped);
	return value;
}

template <typename T>
bool LFStack<T>::empty() const {
	// Could probably go with relaxed memory order
	return head_handle.load(std::memory_order_acquire).head == nullptr;
}

template <typename T>
impl::LinkedListNode<T>* LFStack<T>::get_new_node() {
	constexpr int max_retries = 3;

	impl::LinkedListHandle<T> c_handle = pool_handle.load(std::memory_order_acquire);

	impl::LinkedListNode<T> *popped = nullptr;
	for (int tries = 0; tries < max_retries; ++tries) {
		auto [success, n_popped] = try_pop(pool_handle, c_handle);

		if (success) {
			popped = n_popped;
			break;
		}
	}

	if (popped == nullptr) {
		return new impl::LinkedListNode<T>();
	}
	return popped;
}

/**
 * handle MUST be a reference, otherwise it would not be updated by compare_exchange
 */
template <typename T>
void LFStack<T>::push_to_handle(std::atomic<impl::LinkedListHandle<T>> &handle, impl::LinkedListNode<T> *n_head) {
	impl::LinkedListHandle<T> c_handle = handle.load(std::memory_order_acquire);
	for (;;) {
		impl::LinkedListHandle<T> n_handle = c_handle;
		n_head->next = n_handle.head;
		n_handle.head = n_head;
		++n_handle.tag;

		if (handle.compare_exchange_weak(
				c_handle,
				n_handle,
				std::memory_order_release,
				std::memory_order_acquire)
			){
			break;
		}
	}
}

/**
 * handle and c_handle MUST be references, otherwise they will not be updated by compare_exchange
 */
template <typename T>
std::tuple<bool, impl::LinkedListNode<T>*> LFStack<T>::try_pop(std::atomic<impl::LinkedListHandle<T>> &handle, impl::LinkedListHandle<T> &c_handle) {
	impl::LinkedListHandle<T> n_handle = c_handle;
	impl::LinkedListNode<T> *popped = nullptr;
	if (n_handle.head == nullptr) {
		return {true, nullptr};
	}

	popped = n_handle.head;
	n_handle.head = n_handle.head->next;
	++n_handle.tag;

	if (handle.compare_exchange_weak(
			c_handle,
			n_handle,
			std::memory_order_release,
			std::memory_order_acquire)
		){
		return {true, popped};
	}
	return {false, nullptr};
}

#endif // !NPRG058_HA1_LOCKFREE_GUARD__
