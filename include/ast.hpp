#pragma once
#include "token.hpp"
#include "dynamicarray.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include <string>
#include <memory>
#include <vector>

class AstVisitor;
class ToplevelAstNode;

struct AstNode {
	using Child = std::unique_ptr<AstNode>;
	using Root = std::unique_ptr<ToplevelAstNode>;
	//virtual bool generateCode(Context &ctx, ModuleInfo &mi) = 0;
	virtual void accept(AstVisitor &visitor) = 0;
	void addChild(Child && child) {
		children.push_back(std::move(child) );
	}
	std::vector<Child> children;
};

struct ToplevelAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
};

struct FunctionAstNode : public AstNode {
	FunctionAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	//bool generateCode(Context &ctx, ModuleInfo &mi) override;
	std::string identifier;
};

struct StatementAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	//bool generateCode(Context &ctx, ModuleInfo &mi) override;
};

struct CallAstNode : public AstNode {
	CallAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;

	//bool generateCode(Context &ctx, ModuleInfo &mi) override;
	std::string identifier;
};

struct ExpressionAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	//bool generateCode(Context &ctx, ModuleInfo &mi) override;
};

struct StringAstNode : public AstNode {
	StringAstNode(const std::string &value);
	void accept(AstVisitor &visitor) override;

	//bool generateCode(Context &ctx, ModuleInfo &mi) override;
	std::string value;
};

class AstVisitor {
public:
	virtual void visit(ToplevelAstNode &node)	= 0;
	virtual void visit(FunctionAstNode &node)	= 0;
	virtual void visit(StatementAstNode &node)	= 0;
	virtual void visit(CallAstNode &node)		= 0;
	virtual void visit(ExpressionAstNode &node)	= 0;
	virtual void visit(StringAstNode &node)		= 0;
};

class AstParser {
public:
	AstNode::Root buildTree(Tokens &&tokens);

	//bool generateCode(Context &ctx, ModuleInfo &mi) override;
private:
	Token *getIf(TokenType type);
	AstNode::Root buildTree();
	AstNode::Child buildFunction();
	AstNode::Child buildStatement();
	AstNode::Child buildCall(const std::string &identifier);
	AstNode::Child buildExpr();
	Tokens tokens;
	Tokens::iterator iterator;
};

