#pragma once
#include "ast.hpp"
#include "token.hpp"

#include "llvm/IR/Module.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>

struct ModuleInfo {
	std::string name;
	std::string fileName;
	std::string objName;
	std::unique_ptr<llvm::Module> module;
	Ast ast;
};

void gen(ModuleInfo *mi, Context *ctx);

void tokensToBuilder(ModuleInfo *mi, Context *ctx);

void write(ModuleInfo *mi, Context *ctx);

void link(ModuleInfo *mi, Context *ctx);
