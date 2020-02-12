#pragma once
#include "ast.hpp"
#include "token.hpp"
#include "symtable.hpp"

#include "llvm/IR/Module.h"

#include <iostream>
#include <vector>
#include <stack>
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
	std::unordered_map<std::string, llvm::Function*> functions;
	std::unordered_map<std::string, llvm::FunctionCallee> functionCallees;
	std::unique_ptr<ToplevelAstNode> ast;
	SymTable symtable;
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
	void visit(ToplevelAstNode &node) override;
	void visit(FunctionAstNode &node) override;
	void visit(ExternAstNode &node) override;
	void visit(StatementAstNode &node) override;
	void visit(VariableDeclareAstNode &node) override;
	void visit(CallAstNode &node) override;
	void visit(BinExpressionAstNode &node) override;
	void visit(VariableAstNode &node) override;
	void visit(StringAstNode &node) override;
	void visit(IntAstNode &node) override;
private:
	llvm::Type *translateType(const Type &type) const;
	std::vector<FunctionAstNode*> getFuncsFromToplevel(ToplevelAstNode &node);
	void buildFunctionDefinitions(const std::vector<FunctionAstNode*> &funcs);
	ModuleInfo *mi = nullptr;
	Context *ctx = nullptr;

	std::vector<llvm::Value*> callParams;
	std::vector<VariableAstNode*> visitedVariables;
	std::unordered_map<std::string, llvm::AllocaInst*> locals;
};

bool gen(ModuleInfo *mi, Context *ctx);

void tokensToBuilder(ModuleInfo *mi, Context *ctx);

void write(ModuleInfo *mi, Context *ctx);

void link(ModuleInfo *mi, Context *ctx);
