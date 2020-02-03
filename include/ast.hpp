#pragma once
#include "token.hpp"
#include "dynamicarray.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include <string>
#include <memory>
#include <vector>
#include <unordered_set>

class AstVisitor;
class ToplevelAstNode;
class FunctionAstNode;
class TypeAstNode;

struct AstNode {
	using Child = std::unique_ptr<AstNode>;
	using Root = std::unique_ptr<ToplevelAstNode>;

	virtual ~AstNode() = default;
	virtual void accept(AstVisitor &visitor) = 0;
	void addChild(Child && child);
	std::vector<Child> children;
};

struct ToplevelAstNode : public AstNode {
	using SymTable = std::unordered_set<std::string>;

	void accept(AstVisitor &visitor) override;
	void addFunction(FunctionAstNode *func);
	bool typeExists(const std::string &str) const;

	std::vector<FunctionAstNode*> functions;

	SymTable identifiers;
	SymTable types;
};

struct FunctionAstNode : public AstNode {
	FunctionAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	std::string identifier;
};

struct ExternAstNode : public AstNode {
	ExternAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	std::string identifier;
};

struct StatementAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
};

struct CallAstNode : public AstNode {
	CallAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	std::string identifier;
};

struct ExpressionAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
};

struct TypeAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	std::string name;
	bool isPtr = false;
};

struct StringAstNode : public AstNode {
	StringAstNode(const std::string &value);
	void accept(AstVisitor &visitor) override;
	std::string value;
};

class AstVisitor {
public:
	virtual ~AstVisitor() = default;
	virtual void visit(ToplevelAstNode &node)	= 0;
	virtual void visit(FunctionAstNode &node)	= 0;
	virtual void visit(ExternAstNode &node)		= 0;
	virtual void visit(StatementAstNode &node)	= 0;
	virtual void visit(CallAstNode &node)		= 0;
	virtual void visit(ExpressionAstNode &node)	= 0;
	virtual void visit(TypeAstNode &node)		= 0;
	virtual void visit(StringAstNode &node)		= 0;
};

class AstParser {
public:
	AstNode::Root buildTree(Tokens &&tokens);

private:
	AstNode::Child unexpected() const;
	Token *getIf(TokenType type);
	void discardWhile(TokenType type);
	AstNode::Root buildTree();
	AstNode::Child buildFunction();
	AstNode::Child buildParams();
	AstNode::Child buildExtern();
	AstNode::Child buildStatement();
	AstNode::Child buildCall(const std::string &identifier);
	AstNode::Child buildExpr();
	AstNode::Child buildType();

	Tokens tokens;
	Tokens::iterator iterator;
	ToplevelAstNode *root = nullptr;
};

