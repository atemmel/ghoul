#pragma once
#include "token.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include <string>
#include <memory>
#include <vector>

struct ModuleInfo;

struct Context {
	Context() : builder(context) {};
	llvm::LLVMContext context;
	llvm::IRBuilder<> builder;
};

class AstNode;

class Ast {
public:
	void buildTree(const Tokens &tokens);

	void generateCode(Context &ctx, ModuleInfo &mi);
private:
	CTokenIterator buildFunction(CTokenIterator first, CTokenIterator Last);

	std::unique_ptr<AstNode> root;
};

class AstNode {
public:
	virtual void generateCode(Context &ctx, ModuleInfo &mi) = 0;
	void addChild(std::unique_ptr<AstNode> && child) {
		children.push_back(std::move(child) );
	}
protected:
	std::string value;
	std::vector<std::unique_ptr<AstNode>> children;
};

class FunctionAstNode : public AstNode {
public:
	FunctionAstNode(const std::string &value)  {
		this->value = value;
	}

	void generateCode(Context &ctx, ModuleInfo &mi) override;
};
