#pragma once
#include <vector>
#include <iostream>
#include <algorithm>
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

	ArgParser(int argc, char** argv)
		: args(argv + 1, argv + argc) {}

	void append(std::string_view key, const Command &command) {
		commands.insert({key, command});
	}

	void unwind() {
		Args subArgs;
		for(auto it = args.begin(); it != args.end(); it++) {
			subArgs.clear();
			auto hashIt = commands.find(*it);
			if(hashIt == commands.end() ) {
				std::cerr << "Unrecognized argument: " << *it << ", exiting...\n";
				std::exit(EXIT_FAILURE);
			}
			
			int availableArgs = std::distance(std::next(it), args.end() );
			if(availableArgs < hashIt->second.flags) {
				std::cerr << "Too few arguments for argument " << hashIt->first 
					<< ", expected " << hashIt->second.flags << ", recieved " << availableArgs 
					<< '\n';
				std::exit(EXIT_FAILURE);
			}
			
			subArgs.insert(subArgs.begin(), it + 1, it + 1 + hashIt->second.flags);
			hashIt->second.callback(subArgs);
			it += hashIt->second.flags;
		}
	}

private:
	std::unordered_map<std::string_view, Command> commands;
	Args args;
};
