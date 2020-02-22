#include "type.hpp"
#include "utils.hpp"

unsigned Type::Scope::depth = 0;

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
		Scope scope;
		buffer.push_back('\n');
		for(unsigned i = 0; i < Type::Scope::depth; i++) {
			buffer += "  ";
		}

		buffer += member.type.string();
		buffer.push_back(' ');
		buffer += member.identifier;
	}

	return buffer;
}

Type::Scope::Scope() {
	++Scope::depth;
}

Type::Scope::~Scope() {
	--Scope::depth;
}
