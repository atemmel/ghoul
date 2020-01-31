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
	using Child = std::unique_ptr<AstNode>;
	virtual bool generateCode(Context &ctx, ModuleInfo &mi) = 0;
	void addChild(std::unique_ptr<AstNode> && child) {
		children.push_back(std::move(child) );
	}
protected:
	std::vector<Child> children;
};

class Ast : public AstNode {
public:
	void buildTree(Tokens &&tokens);

	bool generateCode(Context &ctx, ModuleInfo &mi) override;
private:
	Token *getIf(TokenType type);
	void buildTree();
	Child buildFunction();
	Child buildStatement();
	Child buildCall(const std::string &identifier);
	Child buildExpr();
	Tokens tokens;
	Tokens::iterator iterator;
};

class FunctionAstNode : public AstNode {
public:
	FunctionAstNode(const std::string &identifier);

	bool generateCode(Context &ctx, ModuleInfo &mi) override;
private:
	std::string identifier;
};

class StatementAstNode : public AstNode {
public:
	bool generateCode(Context &ctx, ModuleInfo &mi) override;
};

class CallAstNode : public AstNode {
public:
	CallAstNode(const std::string &identifier);

	bool generateCode(Context &ctx, ModuleInfo &mi) override;
private:
	std::string identifier;
};

class ExpressionAstNode : public AstNode {
public:
	bool generateCode(Context &ctx, ModuleInfo &mi) override;
};

class StringAstNode : public AstNode {
public:
	StringAstNode(const std::string &value);

	bool generateCode(Context &ctx, ModuleInfo &mi) override;
	std::string value;
private:
};
