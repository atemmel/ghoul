#pragma once
#include <memory>
#include <string>
#include <vector>

struct Member;

struct Type {
	Type() = default;
	Type(const Type &rhs);
	Type operator=(Type rhs);

	bool operator==(const Type &rhs) const;
	bool operator!=(const Type &rhs) const;

	void swap(Type &rhs);

	std::string string() const;
	std::string fullString() const;
	int size() const;

	//TODO: Reorder struct members
	std::string name;
	unsigned isPtr = 0;
	std::unique_ptr<Type> arrayOf;
	std::vector<Member> members;
};

struct Member {
	std::string identifier;
	Type type;
};
