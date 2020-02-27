#include "type.hpp"
#include "utils.hpp"

#include <algorithm>
#include <iostream>

bool Type::operator==(const Type &rhs) const {
	return name == rhs.name && isPtr == rhs.isPtr;
}

bool Type::operator!=(const Type &rhs) const {
	return !(*this == rhs);
}

std::string Type::string() const {
	if(name.empty() ) {
		return "<unresolved>";
	}

	std::string buffer = name;
	if(isPtr) {
		buffer += " *";
	}

	for(const auto &member : members) {
		buffer.push_back('\n');
		buffer += member.type.string();
		buffer.push_back(' ');
		buffer += member.identifier;
	}

	return buffer;
}
