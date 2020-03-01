#include "frontend.hpp"

#include "astprint.hpp"
#include "clock.hpp"
#include "global.hpp"
#include "lexer.hpp"
#include "utils.hpp"

#include <filesystem>

static std::string findLibPath(const std::string &origin);

static void displayTokens(const Tokens &tokens);

AstNode::Root performFrontendWork(const std::string &module, SymTable *symtable) {
	std::string filename = module + ".gh";	//TODO: Remove hardcoded constant
	if(!std::filesystem::exists(filename) ) {
		std::string libPath = findLibPath(module);
		if(libPath.empty() ) {
			Global::errStack.push("Could not find module '" + module + "'", nullptr);
			Global::errStack.unwind();
			exit(EXIT_FAILURE);
		}

		filename = libPath + '/' + filename;
		std::cerr << filename << '\n';
	}

	float time;
	Clock clock;
	auto str = consumeFile(filename.c_str() );
	if(str.empty() ) {
		Global::errStack.push("File is either empty or does not exist", nullptr);
		Global::errStack.unwind();
		exit(EXIT_FAILURE);
	}

	time = clock.getNanoSeconds();
	std::cout << filename << " read in " << time << " ns\n";

	clock.restart();
	Lexer lexer;
	auto tokens = lexer.lexTokens(str);
	time = clock.getNanoSeconds();
	std::cout << filename << " tokenized in " << time << " ns\n";
	if(tokens.empty() ) {
		Global::errStack.push("File does not contain any valid tokens", nullptr);
	}
	if(Global::config.verbose) displayTokens(tokens);
	if(!Global::errStack.empty() ) {
		Global::errStack.unwind();
		std::cerr << "Tokenization step failed\n";
		exit(EXIT_FAILURE);
	}

	clock.restart();
	AstParser parser;
	AstNode::Root ast = std::move(parser.buildTree(std::move(tokens), symtable) );

	time = clock.getNanoSeconds();
	std::cout << filename << " ast built in " << time << " ns\n";
	if(!Global::errStack.empty() ) {
		Global::errStack.unwind();
		std::cerr << "Parsing step failed\n";
		exit(EXIT_FAILURE);
	}

	if(Global::config.verbose || Global::config.verboseAst) {
		AstPrinter().visit(*ast);
	}

	clock.restart();
	symtable->visit(*ast);
	time = clock.getNanoSeconds();
	std::cout << filename << " symbol pass completed in " << time << " ns\n";
	if(Global::config.verbose || Global::config.verboseSymtable) {
		symtable->dump();
	}
	if(!Global::errStack.empty() ) {
		Global::errStack.unwind();
		std::cerr << "Symbol pass failed\n";
		exit(EXIT_FAILURE);
	}

	return ast;
}

static std::string findLibPath(const std::string &origin) {
	auto cwd = std::filesystem::current_path();
	auto path = cwd.filename();

	while(!path.empty() && path != "ghoul") { //TODO: Remove hardcoded constant
		cwd = cwd.parent_path();
		path = cwd.filename();
	}

	if(path.empty() ) {
		return "";
	}

	cwd /= "lib";
	if(!std::filesystem::exists(cwd) ) {
		return "";
	}

	return cwd;
}

static void displayTokens(const Tokens &tokens) {
	for(const Token &token : tokens) {
		std::cout << "Index: " << token.index << " Row: "
			<< token.row << " Col: " << token.col << ", Type: " 
			<< token.getPrintString() << " (" 
			<< static_cast<size_t>(token.type) << "), Value: "
			<< token.value << '\n';
	}
}
