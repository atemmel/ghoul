#pragma once
#include "utils.hpp"

#include <array>
#include <algorithm>

template<typename T, size_t capacity>
class DynamicArray {
public:
	//Ctors

	DynamicArray() 
		: len(0) {
	}

	DynamicArray(size_t size, const T& value) 
		: len(size) {
		verboseAssert(len <= capacity, "size argument exceeded maximum capacity");
		std::fill(begin(), end(), value);
	}

	DynamicArray(std::initializer_list<T> list) 
		: len(list.size() ) {
		verboseAssert(len <= capacity, "size argument exceeded maximum capacity");
		std::copy(begin(), end(), list.begin() );
	}

	//Insertion/removal

	void push(const T &value) {
		verboseAssert(len < capacity, "push exceeded maximum capacity");
		array[len++] = value;
	}

	void pop() {
		verboseAssert(len == 0, "pop exceeded minimum capacity");
		--len;
	}

	void clear() {
		len = 0;
	}

	//Accessors

	T &operator[](const size_t index) {
		verboseAssert(index < len, "index out of range");
		return array[index];
	}

	const T &operator[](const size_t index) const {
		verboseAssert(index < len, "index out of range");
		return array[index];
		return array[index];
	}

	T &front() {
		verboseAssert(len > 0, "front element does not exist");
		return array.front();
	}

	T &back() {
		verboseAssert(len > 0, "back element does not exist");
		return array[len - 1];
	}

	const T &front() const {
		verboseAssert(len > 0, "front element does not exist");
		return array.front();
	}

	const T &back() const {
		verboseAssert(len > 0, "back element does not exist");
		return array[len - 1];
	}

	//Special accessors
	
	auto find(const T &value) {
		return std::find(begin(), end(), value);
	}

	auto find(const T &value) const {
		return std::find(begin(), end(), value);
	}

	bool exists (const T &value) const {
		return std::find(begin(), end(), value) != end();
	}

	//Iterators

	auto begin() {
		return array.begin();
	}

	auto end() {
		//Returning array.end() makes this class redundant
		return array.begin() + len;
	}

	auto begin() const {
		return array.cbegin();
	}

	auto end() const {
		//Returning array.cend() makes this class redundant
		return array.cbegin() + len;
	}

	//Size

	size_t size() const {
		return len;
	}

	bool empty() const {
		return len == 0;
	}

	constexpr size_t maxSize() {
		return array.size();
	}

private:
	std::array<T, capacity> array;
	size_t len;
};
