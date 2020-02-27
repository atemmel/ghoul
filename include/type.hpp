#pragma once
#include <string>
#include <vector>

struct Member;

struct Type {
	bool operator==(const Type &rhs) const;
	bool operator!=(const Type &rhs) const;
	std::string string() const;

	std::string name;
	bool isPtr = false;	//TODO: Refactor as unsigned
	std::vector<Member> members;
};

struct Member {
	std::string identifier;
	Type type;
};
