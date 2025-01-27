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
	std::vector<ToplevelAstNode*> toplevels;

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
	bool isVolatile = false;
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
	bool visited = false;
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
	AstNode::Child loopPrefix;
	AstNode::Child loopSuffix;
};

struct ExpressionAstNode : public AstNode {
	int precedence = 0;
};

struct CallAstNode : public ExpressionAstNode {
	CallAstNode(const std::string &identifier);
	void accept(AstVisitor &visitor) override;
	std::string identifier;
	bool isCast = false;
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

struct CastExpressionAstNode : public ExpressionAstNode {
	CastExpressionAstNode(const Type &type);
	void accept(AstVisitor &visitor) override;
	Type type;
};

struct ArrayAstNode : public ExpressionAstNode {
	void accept(AstVisitor &visitor) override;
	AstNode::Expr length;
	Type type;
	bool raArray = false;
};

struct IndexAstNode : public ExpressionAstNode { 
	void accept(AstVisitor &visitor) override;
	AstNode::Expr index;
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
	virtual void visit(CastExpressionAstNode &node)		= 0;
	virtual void visit(ArrayAstNode &node)				= 0;
	virtual void visit(IndexAstNode &node)				= 0;
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

	AstNode::Root buildTree();
	AstNode::Root buildImport();
	AstNode::Child buildLink();
	AstNode::Child buildStruct();
	AstNode::Child buildFunction();
	AstNode::Child buildParams();
	AstNode::Child buildExtern();
	AstNode::Child buildStatement();
	AstNode::Child buildDecl();
	AstNode::Child buildBranch();
	AstNode::Child buildLoop();
	AstNode::Child buildWhile();
	AstNode::Child buildFor();
	AstNode::Expr buildCall(const std::string &identifier);
	AstNode::Expr buildExpr();
	AstNode::Expr buildPrimaryExpr();
	AstNode::Expr buildVariableExpr(Token *token);
	AstNode::Expr buildMemberExpr();
	AstNode::Expr buildAssignExpr(AstNode::Expr &lhs);
	AstNode::Expr buildBinOp();
	AstNode::Expr buildBinExpr(AstNode::Expr &child);
	AstNode::Expr buildPrefixUnaryOp();
	AstNode::Expr buildPrefixUnaryExpr();
	AstNode::Expr buildPostfixUnaryOp();
	AstNode::Expr buildPostfixUnaryExpr(AstNode::Expr &child);
	AstNode::Expr buildCast();
	AstNode::Expr buildArray();
	AstNode::Expr buildIndex();

	//Type buildType(Token *token);
	bool buildType(Type &type);
	bool buildSubType(Type &type);

	template<typename T>
	AstNode::Expr toExpr(std::unique_ptr<T> ptr) {
		return nullptr;
	}

	template<typename T>
	AstNode::Root toRoot(std::unique_ptr<T> ptr) {
		return nullptr;
	}

	std::vector<Token> tokens;
	Tokens::iterator iterator;
	ToplevelAstNode *root = nullptr;
	SymTable *symtable = nullptr;

	bool mayParseAssign = true;
	bool isPanic = false;
};
