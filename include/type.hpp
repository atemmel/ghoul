#pragma once
#include <string>
#include <vector>

struct Member;

struct Type {
	bool operator==(const Type &rhs) const;
	bool operator!=(const Type &rhs) const;
	std::string string() const;

	//TODO: Reorder struct members
	std::string name;
	unsigned isPtr = 0;	//TODO: Refactor as unsigned
	std::vector<Member> members;
};

struct Member {
	std::string identifier;
	Type type;
};
