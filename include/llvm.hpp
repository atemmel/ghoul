#pragma once
#include "ast.hpp"
#include "token.hpp"

#include "llvm/IR/Module.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

struct ModuleInfo {
	using ValueMap = std::unordered_map<std::string, llvm::Value*>;
	std::string name;
	std::string fileName;
	std::string objName;
	std::unique_ptr<llvm::Module> module;
	ValueMap values;
	std::unordered_map<std::string, llvm::FunctionCallee> functions;
	std::unique_ptr<ToplevelAstNode> ast;
};

struct Context {
	Context() : builder(context) {};
	llvm::LLVMContext context;
	llvm::IRBuilder<> builder;
};


class LLVMCodeGen : public AstVisitor {
public:
	void setModuleInfo(ModuleInfo *mi);
	void setContext(Context *ctx);
	virtual void visit(ToplevelAstNode &node) override;
	virtual void visit(FunctionAstNode &node) override;
	virtual void visit(StatementAstNode &node) override;
	virtual void visit(CallAstNode &node) override;
	virtual void visit(ExpressionAstNode &node) override;
	virtual void visit(StringAstNode &node) override;
private:
	ModuleInfo *mi = nullptr;
	Context *ctx = nullptr;

	//TODO: Refactor this
	ModuleInfo::ValueMap::iterator activeValue;
};

bool gen(ModuleInfo *mi, Context *ctx);

void tokensToBuilder(ModuleInfo *mi, Context *ctx);

void write(ModuleInfo *mi, Context *ctx);

void link(ModuleInfo *mi, Context *ctx);
