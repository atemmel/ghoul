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
#include "frontend.hpp"


void buildModuleInfo(ModuleInfo &mi, std::string_view sv) {
	mi.fileName = sv;
	mi.name = endsWith(sv, ".gh") ? removeStem(sv) : sv;
	mi.objName = mi.name + ".o";
}

void compile(ModuleInfo &mi) {
	static Context ctx;
	SymTable symtable;
	mi.symtable = &symtable;

	mi.ast = performFrontendWork(mi.fileName, mi.symtable);

	float time;
	Clock clock;

	if(!gen(&mi, &ctx) ) {
		exit(EXIT_FAILURE);
	}

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
	argParser.addBool(&Global::config.verboseAst, "--verbose-ast");
	argParser.addBool(&Global::config.verboseSymtable, "--verbose-symtable");
	argParser.addBool(&Global::config.verboseIR, "--verbose-ir");

	argParser.unwind();

	if(!buildFlag.empty() ) {
		buildModuleInfo(mi, buildFlag);
		Global::errStack.setFile(&mi.fileName);
		compile(mi);
	}
}
