#include "argparser.hpp"

#include <iostream>
#include <algorithm>

ArgParser::ArgParser(int argc, char** argv) : args(argv + 1, argv + argc) {
}

void ArgParser::append(std::string_view key, const Command &command) {
	commands.insert({key, command});
}

void ArgParser::unwind() {
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
