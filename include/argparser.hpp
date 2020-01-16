#pragma once
#include <vector>
#include <functional>
#include <string_view>
#include <unordered_map>

class ArgParser {
public:
	using Args = std::vector<std::string_view>;
	using Callback = std::function<void(const Args&)>;

	struct Command {
		Callback callback;
		int flags = 0;
	};

	ArgParser(int argc, char** argv);

	void append(std::string_view key, const Command &command);

	void unwind();

private:
	std::unordered_map<std::string_view, Command> commands;
	Args args;
};
