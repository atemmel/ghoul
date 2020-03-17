#include "type.hpp"
#include "utils.hpp"

#include <algorithm>
#include <iostream>

bool Type::operator==(const Type &rhs) const {
	return name == rhs.name && isPtr == rhs.isPtr && isArray == rhs.isArray;
}

bool Type::operator!=(const Type &rhs) const {
	return !(*this == rhs);
}

std::string Type::string() const {
	if(name.empty() ) {
		return "<unresolved>";
	}

	std::string buffer = name + ' ';
	for(int i = 0; i < isPtr; i++) {
		buffer += "*";
	}

	for(const auto &member : members) {
		buffer.push_back('\n');
		buffer += member.type.string();
		buffer.push_back(' ');
		buffer += member.identifier;
	}

	return buffer;
}
