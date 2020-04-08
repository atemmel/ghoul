#include "type.hpp"
#include "utils.hpp"

#include <algorithm>
#include <iostream>

Type::Type(const Type &rhs) {
	name = rhs.name;
	isPtr = rhs.isPtr;
	members = rhs.members;
	if(rhs.arrayOf) {
		arrayOf = std::make_unique<Type>(*rhs.arrayOf);
	}
}

Type Type::operator=(Type rhs) {
	swap(rhs);
	return *this;
}

bool Type::operator==(const Type &rhs) const {
	if(arrayOf && !(rhs.arrayOf && *arrayOf == *rhs.arrayOf) ) {
		return false;
	}

	return name == rhs.name && isPtr == rhs.isPtr;
}

bool Type::operator!=(const Type &rhs) const {
	return !(*this == rhs);
}

void Type::swap(Type &rhs) {
	std::swap(name, rhs.name);
	std::swap(isPtr, rhs.isPtr);
	std::swap(members, rhs.members);
	std::swap(arrayOf, rhs.arrayOf);
}

std::string Type::string() const {
	std::string buffer = name;
	if(arrayOf) {
		buffer += "[]";
		if(realignedArray) {
			buffer += "@";
		}
	}

	for(int i = 0; i < isPtr; i++) {
		buffer.push_back('*');
	}

	if(arrayOf) {
		buffer += arrayOf->string();
	}

	if(buffer.empty() ) {
		return "<unresolved>";
	}
	return buffer;
}

std::string Type::fullString() const {
	if(name.empty() ) {
		return "<unresolved>";
	}

	std::string buffer = name;
	for(const auto &member : members) {
		buffer.push_back('\n');
		buffer += member.type.string();
		buffer.push_back(' ');
		buffer += member.identifier;
	}

	return buffer;
}

int Type::size() const {
	if(isPtr > 0) {
		return 8;
	}

	if(arrayOf) {
		return 8 + 4 + 4;
	}

	if(!members.empty() ) {
		int sum = 0;
		for(auto &member : members) {
			sum += member.type.size();
		}
		return sum;
	}

	if(name == "int") {
		return 4;
	} else if(name == "float") {
		return 4;
	} else if(name == "char") {
		return 1;
	} else if(name == "void") {
		return 0;
	}

	return 0;
}

bool Type::isStruct() const {
	if(name == "int" 
			|| name == "float" 
			|| name == "char" 
			|| name == "void") {
		return false;
	}

	return true;
}
