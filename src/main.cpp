#include <algorithm>
#include <iostream>

#include "lexer.hpp"
#include "llvm.hpp"
#include "utils.hpp"
#include "clock.hpp"
#include "token.hpp"
#include "global.hpp"
#include "argparser.hpp"
#include "astprint.hpp"

void displaySource(const std::string &str) {
	int line = 1;
	int column = 1;
	int maxLine = 1;
	int maxColumn = 0;
	std::string lineStr;
	std::string columnStr;

	for(const auto c : str) {
		if(c == '\n') {
			++maxLine;
			column = 0;
		}
		++column;

		if(column > maxColumn) {
			maxColumn = column;
		}
	}

	column = 1;
	auto lineDigits = std::to_string(maxLine).size();
	auto columnDigits = std::to_string(maxColumn).size();

	auto pad = [](const size_t n) {
		for(size_t i = 0; i < n; i++) {
			std::cout << ' ';
		}
	};

	for(const auto c : str) {
		lineStr = std::to_string(line);
		columnStr = std::to_string(column);
		std::cout << "L: ";
		pad(lineDigits - lineStr.size() );
		std::cout << lineStr;
		std::cout << " C: ";
		pad(columnDigits - columnStr.size() ); 
		std::cout << columnStr << ' ';

		if(c == '\n') {
			std::cout << "'\\n'\n";
			++line;
			column = 0;
		} else if(c == ' ') {
			std::cout << "' '\n";
		} else if(c == '\t') {
			std::cout << "'\\t'\n";
		} else {
			std::cout << '\'' << c << '\'' << '\n';
		}

		++column;
	}
}

void displayTokens(const Tokens &tokens) {
	for(const Token &token : tokens) {
		std::cout << "Index: " << token.index << " Row: "
			<< token.row << " Col: " << token.col << ", Type: " 
			<< token.getPrintString() << " (" 
			<< static_cast<size_t>(token.type) << "), Value: "
			<< token.value << '\n';
	}
}

void buildModuleInfo(ModuleInfo &mi, std::string_view sv) {
	if(!endsWith(sv, ".scp") ) {	//TODO: Remove hardcoded constant ".scp"
		std::cerr << "Invalid source file given, expected a source file with a \".scp\" file extension\n";
		std::exit(EXIT_FAILURE);
	}

	mi.fileName = sv;
	mi.name = removeStem(getFileName(sv) );
	mi.objName = mi.name + ".o";
}

void compile(ModuleInfo &mi) {
	static Context ctx;

	float time;
	Clock clock;
	auto str = consumeFile(mi.fileName.c_str() );
	if(str.empty() ) {
		Global::errStack.push("File does not exist", nullptr);
		Global::errStack.unwind();
		return;
	}

	time = clock.getNanoSeconds();
	std::cout << mi.fileName.c_str() << " read in " << time << " ns\n";
	//displaySource(str);

	clock.restart();
	Lexer lexer;
	auto tokens = lexer.lexTokens(str);
	time = clock.getNanoSeconds();
	std::cout << mi.fileName.c_str() << " tokenized in " << time << " ns\n";
	if(tokens.empty() ) {
		Global::errStack.push("File does not contain any valid tokens", nullptr);
	}
	if(Global::config.verbose) displayTokens(tokens);
	if(!Global::errStack.empty() ) {
		Global::errStack.unwind();
		std::cerr << "Tokenization step failed\n";
		return;
	}

	clock.restart();
	AstParser parser;
	mi.ast = std::move(parser.buildTree(std::move(tokens) ) );
	time = clock.getNanoSeconds();
	std::cout << mi.fileName.c_str() << " ast built in " << time << " ns\n";
	if(!Global::errStack.empty() ) {
		Global::errStack.unwind();
		std::cerr << "Parsing step failed\n";
		return;
	}

	if(Global::config.verbose) {
		AstPrinter().visit(*mi.ast);
	}

	clock.restart();
	mi.symtable.visit(*mi.ast);
	time = clock.getNanoSeconds();
	std::cout << mi.fileName.c_str() << " symbol pass completed in " << time << " ns\n";
	if(!Global::errStack.empty() ) {
		Global::errStack.unwind();
		std::cerr << "Symbol pass failed\n";
		return;
	}

	clock.restart();
	if(!gen(&mi, &ctx) ) return;
	time = clock.getMilliSeconds();
	std::cout << mi.objName.c_str() << " object file built in " << time << " ms\n";
}

int main(int argc, char** argv) {
#ifdef DEBUG
	float f;
	verboseAssert(isFloatLiteral("127.5", f) == NumValidity::Ok,      "Ok test failed");
	verboseAssert(isFloatLiteral("127r5", f) == NumValidity::Invalid, "Invalid test failed");
	verboseAssert(isFloatLiteral("1e700", f) == NumValidity::Range,   "Range test failed");

	int i;
	verboseAssert(isIntLiteral("12345", i) == NumValidity::Ok,      "Ok test failed");
	verboseAssert(isIntLiteral("123.4", i) == NumValidity::Invalid, "Invalid test failed");
	verboseAssert(isIntLiteral("5000000000", i) == NumValidity::Range,   "Range test failed");
	std::cout << "All assertions passed\n";
#endif

	ModuleInfo mi;
	std::string buildFlag;

	ArgParser argParser(argc, argv);
	argParser.addString(&buildFlag, "build");
	argParser.addBool(&Global::config.verbose, "--verbose");

	argParser.unwind();

	if(!buildFlag.empty() ) {
		buildModuleInfo(mi, buildFlag);
		Global::errStack.setFile(&mi.fileName);
		compile(mi);
	}
}
