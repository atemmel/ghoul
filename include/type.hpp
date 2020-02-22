#pragma once
#include <string>
#include <vector>

struct Member;

struct Type {
	bool operator==(const Type &rhs) const;
	bool operator!=(const Type &rhs) const;
	std::string string() const;

	std::string name;
	bool isPtr = false;
	std::vector<Member> members;

private:
	struct Scope {
		Scope();
		~Scope();
		static unsigned depth;
	};
};

struct Member {
	std::string identifier;
	Type type;
};
