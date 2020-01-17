#pragma once
#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>

class ArgParser {
public:
	using Args = std::vector<std::string_view>;

	ArgParser(int argc, char** argv);

	void addBool(bool* var, std::string_view flag);
	void addString(std::string* var, std::string_view flag);

	void unwind();

private:
	struct VarPtr {
		enum struct Type {
			Bool,
			String
		};
		void* ptr;
		Type type;
	};

	std::unordered_map<std::string_view, VarPtr> flags;
	Args args;
};
