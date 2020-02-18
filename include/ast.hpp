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
class ExternAstNode;

struct Type {
	bool operator==(const Type &rhs) const {
		return name == rhs.name && isPtr == rhs.isPtr;
	}
	bool operator!=(const Type &rhs) const {
		return !(*this == rhs);
	}
	std::string string() const {
		if(name.empty() ) {
			return "<unresolved>";
		}
		if(isPtr) {
			return name + " *";
		}
		return name;
	}
	std::string name;
	bool isPtr = false;
};

struct FunctionSignature {
	std::string name;
	Type returnType;
	std::vector<Type> parameters;
	std::vector<std::string> paramNames;
};

struct AstNode {
	using Child = std::unique_ptr<AstNode>;
	using Root = std::unique_ptr<ToplevelAstNode>;

	virtual ~AstNode() = default;
	virtual void accept(AstVisitor &visitor) = 0;
	void addChild(Child && child);
	std::vector<Child> children;
	Token *token = nullptr;
};

struct ToplevelAstNode : public AstNode {
	using SymTable = std::unordered_set<std::string>;

	void accept(AstVisitor &visitor) override;
	void addFunction(FunctionAstNode *func);
	void addExtern(ExternAstNode *func);

	std::vector<FunctionAstNode*> functions;
	std::vector<ExternAstNode*> externs;
};

struct FunctionAstNode : public AstNode {
	FunctionAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	FunctionSignature signature;
};

struct ExternAstNode : public AstNode {
	ExternAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	FunctionSignature signature;
	std::string name;
};

struct StatementAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
};

struct VariableDeclareAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	Type type;
	std::string identifier;
};

struct ReturnAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
};

struct ExpressionAstNode : public AstNode {
	int precedence = 0;
};

struct CallAstNode : public ExpressionAstNode {
	CallAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	std::string identifier;
};

struct BinExpressionAstNode : public ExpressionAstNode {
	BinExpressionAstNode(TokenType type);
	void accept(AstVisitor &visitor) override;
	TokenType type;
};

struct VariableAstNode : public ExpressionAstNode {
	VariableAstNode(const std::string &name);
	void accept(AstVisitor &visitor) override;
	std::string name;
};

struct StringAstNode : public ExpressionAstNode {
	StringAstNode(const std::string &value);
	void accept(AstVisitor &visitor) override;
	std::string value;
};

struct IntAstNode : public ExpressionAstNode {
	IntAstNode(const std::string &value);
	void accept(AstVisitor &visitor) override;
	int value;
};

class AstVisitor {
public:
	virtual ~AstVisitor() = default;
	virtual void visit(ToplevelAstNode &node)			= 0;
	virtual void visit(FunctionAstNode &node)			= 0;
	virtual void visit(ExternAstNode &node)				= 0;
	virtual void visit(StatementAstNode &node)			= 0;
	virtual void visit(VariableDeclareAstNode &node)	= 0;
	virtual void visit(ReturnAstNode &node)				= 0;
	virtual void visit(CallAstNode &node)				= 0;
	virtual void visit(BinExpressionAstNode &node)		= 0;
	virtual void visit(VariableAstNode &node)			= 0;
	virtual void visit(StringAstNode &node)				= 0;
	virtual void visit(IntAstNode &node)				= 0;
};

class AstParser {
public:
	AstNode::Root buildTree(Tokens &&tokens);

private:
	AstNode::Child unexpected() const;
	Token *getIf(TokenType type);
	void unget();
	void discardWhile(TokenType type);
	AstNode::Root buildTree();
	AstNode::Child buildFunction();
	AstNode::Child buildParams();
	AstNode::Child buildExtern();
	AstNode::Child buildStatement();
	std::unique_ptr<ExpressionAstNode> buildCall(const std::string &identifier);
	std::unique_ptr<ExpressionAstNode> buildExpr();
	std::unique_ptr<ExpressionAstNode> buildPrimaryExpr();
	std::unique_ptr<ExpressionAstNode> buildAssignExpr(std::unique_ptr<ExpressionAstNode> &lhs);
	std::unique_ptr<ExpressionAstNode> buildBinOp();
	std::unique_ptr<ExpressionAstNode> buildBinExpr(std::unique_ptr<ExpressionAstNode> &child);

	template<typename T>
	std::unique_ptr<ExpressionAstNode> toExpr(std::unique_ptr<T> ptr) {
		return nullptr;
	}

	Tokens tokens;
	Tokens::iterator iterator;
	ToplevelAstNode *root = nullptr;
	bool mayParseAssign = true;
};
