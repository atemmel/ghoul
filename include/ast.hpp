#pragma once
#include "token.hpp"
#include "type.hpp"

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
class StructAstNode;
class ExpressionAstNode;
class StringAstNode;

class SymTable;

struct FunctionSignature {
	std::string name;
	Type returnType;
	std::vector<Type> parameters;
	std::vector<std::string> paramNames;
};

struct AstNode {
	using Child = std::unique_ptr<AstNode>;
	using Root = std::unique_ptr<ToplevelAstNode>;
	using Expr = std::unique_ptr<ExpressionAstNode>;

	virtual ~AstNode() = default;
	virtual void accept(AstVisitor &visitor) = 0;
	void addChild(Child && child);
	std::vector<Child> children;
	Token *token = nullptr;
};

struct ToplevelAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	void addFunction(FunctionAstNode *func);
	void addExtern(ExternAstNode *func);

	std::vector<FunctionAstNode*> functions;
	std::vector<ExternAstNode*> externs;
	std::vector<StructAstNode*> structs;

	bool analyzed = false;
};

struct LinkAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	StringAstNode *string = nullptr;
};

struct StructAstNode : public AstNode {
	StructAstNode(const std::string &name);
	void accept(AstVisitor &visitor) override;
	std::string name;
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

struct VariableDeclareAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	Type type;
	std::string identifier;
};

struct ReturnAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
};

struct BranchAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	AstNode::Expr expr;
};

struct LoopAstNode : public AstNode {
	void accept(AstVisitor &visitor) override;
	AstNode::Expr expr;
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

struct UnaryExpressionAstNode : public ExpressionAstNode {
	UnaryExpressionAstNode(Token *token);
	void accept(AstVisitor &visitor) override;
	TokenType type;
};

struct MemberVariableAstNode : public ExpressionAstNode {
	MemberVariableAstNode(const std::string &name);
	void accept(AstVisitor &visitor) override;
	std::string name;
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
	IntAstNode(int value);
	IntAstNode(const std::string &value);
	void accept(AstVisitor &visitor) override;
	int value;
};

struct BoolAstNode : public ExpressionAstNode {
	BoolAstNode(bool value);
	void accept(AstVisitor &visitor) override;
	bool value;
};

class AstVisitor {
public:
	virtual ~AstVisitor() = default;
	virtual void visit(ToplevelAstNode &node)			= 0;
	virtual void visit(LinkAstNode &node)				= 0;
	virtual void visit(StructAstNode &node)				= 0;
	virtual void visit(FunctionAstNode &node)			= 0;
	virtual void visit(ExternAstNode &node)				= 0;
	virtual void visit(VariableDeclareAstNode &node)	= 0;
	virtual void visit(ReturnAstNode &node)				= 0;
	virtual void visit(BranchAstNode &node)				= 0;
	virtual void visit(LoopAstNode &node)				= 0;
	virtual void visit(CallAstNode &node)				= 0;
	virtual void visit(BinExpressionAstNode &node)		= 0;
	virtual void visit(UnaryExpressionAstNode &node)	= 0;
	virtual void visit(MemberVariableAstNode &node)		= 0;
	virtual void visit(VariableAstNode &node)			= 0;
	virtual void visit(StringAstNode &node)				= 0;
	virtual void visit(IntAstNode &node)				= 0;
	virtual void visit(BoolAstNode &node)				= 0;
};

class AstParser {
public:
	AstNode::Root buildTree(Tokens &&tokens, SymTable *symtable);

private:

#ifndef NDEBUG	//Debug
#define unexpected() \
	panic(__FILE__, __LINE__)
	AstNode::Child panic(const char *file, int line);
#else	//Release
#define unexpected() \
	panic()
	AstNode::Child panic();
#endif

	Token *getIf(TokenType type);
	void unget();
	void discardWhile(TokenType type);
	void discardUntil(TokenType type);
	AstNode::Root mergeTrees(AstNode::Root &&lhs, AstNode::Root &&rhs);
	AstNode::Root buildTree();
	AstNode::Root buildImport();
	AstNode::Child buildLink();
	AstNode::Child buildStruct();
	AstNode::Child buildFunction();
	AstNode::Child buildParams();
	AstNode::Child buildExtern();
	AstNode::Child buildStatement();
	AstNode::Child buildDecl(Token *token);
	AstNode::Child buildBranch();
	AstNode::Child buildLoop();
	AstNode::Expr buildCall(const std::string &identifier);
	AstNode::Expr buildExpr();
	AstNode::Expr buildPrimaryExpr();
	AstNode::Expr buildVariableExpr(Token *token);
	AstNode::Expr buildMemberExpr();
	AstNode::Expr buildAssignExpr(AstNode::Expr &lhs);
	AstNode::Expr buildBinOp();
	AstNode::Expr buildBinExpr(AstNode::Expr &child);
	AstNode::Expr buildUnaryOp();
	AstNode::Expr buildUnaryExpr();

	Type buildType(Token *token);

	template<typename T>
	AstNode::Expr toExpr(std::unique_ptr<T> ptr) {
		return nullptr;
	}

	template<typename T>
	AstNode::Root toRoot(std::unique_ptr<T> ptr) {
		return nullptr;
	}

	Tokens tokens;
	Tokens::iterator iterator;
	ToplevelAstNode *root = nullptr;
	SymTable *symtable = nullptr;
	bool mayParseAssign = true;
	bool isPanic = false;
};
