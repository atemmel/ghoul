#pragma once
#include "token.hpp"
#include "dynamicarray.hpp"

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

class AstNode {
public:
	virtual void generateCode(Context &ctx, ModuleInfo &mi) = 0;
	void addChild(std::unique_ptr<AstNode> && child) {
		children.push_back(std::move(child) );
	}
protected:
	std::vector<std::unique_ptr<AstNode>> children;
};

class Ast : public AstNode {
public:
	void buildTree(Tokens &&tokens);

	void generateCode(Context &ctx, ModuleInfo &mi) override;
private:
	bool expect(TokenType type);
	void buildTree();
	CTokenIterator buildFunction(CTokenIterator it);
	CTokenIterator buildStatement(CTokenIterator it);

	DynamicArray<TokenType, 4> expectedArray;	//Preliminary number, take care
	using Expected = decltype(expectedArray.begin() );
	Expected expected = expectedArray.begin();
	Tokens tokens;
};

class FunctionAstNode : public AstNode {
public:
	FunctionAstNode(const std::string &identifier);

	void generateCode(Context &ctx, ModuleInfo &mi) override;
private:
	std::string identifier;
};

class CallAstNode : public AstNode {
public:
	CallAstNode(const std::string &identifier);

	void generateCode(Context &ctx, ModuleInfo &mi) override;
private:
	std::string identifier;
};
