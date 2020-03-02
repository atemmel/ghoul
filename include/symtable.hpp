#pragma once
#include "ast.hpp"
#include "global.hpp"

#include <unordered_map>
#include <unordered_set>

class SymTable : public AstVisitor {
public:
	SymTable();

	void dump() const;

	bool pushFunc(const std::string &identifier, FunctionSignature *func);

	void setActiveFunction(const std::string &str);
	const Type *getLocal(const std::string &str) const;

	const FunctionSignature *hasFunc(const std::string &identifier) const;

	bool hasType(const std::string &identifier) const;

	const Type* hasStruct(const std::string &identifier) const;
	const Type* typeHasMember(const Type &type, const std::string &identifier) const;
	unsigned getMemberOffset(const Type &type, const std::string &identifier) const;

	void visit(ToplevelAstNode &node) override;
	void visit(LinkAstNode &node) override;
	void visit(StructAstNode &node) override;
	void visit(FunctionAstNode &node) override;
	void visit(ExternAstNode &node) override;
	void visit(VariableDeclareAstNode &node) override;
	void visit(ReturnAstNode &node) override;
	void visit(BranchAstNode &node) override;
	void visit(LoopAstNode &node) override;
	void visit(CallAstNode &node) override;
	void visit(BinExpressionAstNode &node) override;
	void visit(UnaryExpressionAstNode &node) override;
	void visit(CastExpressionAstNode &node) override;
	void visit(MemberVariableAstNode &node) override;
	void visit(VariableAstNode &node) override;
	void visit(StringAstNode &node) override;
	void visit(IntAstNode &node) override;
	void visit(BoolAstNode &node) override;

private:
	enum struct Context {
		InsideStruct,
		InsideFunction
	};

	template <typename T>
	using Map = std::unordered_map<std::string, T>;
	using Set = std::unordered_set<std::string>;
	using Locals = Map<const Type*>;
	using Structs = Map<Type>;

	bool demoteExprToBool(AstNode::Expr &expr);
	bool resolveCast(CallAstNode &call);

	Context context;
	Set types;
	Structs structs;
	Map<const FunctionSignature*> functions;
	const FunctionSignature *currentFunction = nullptr;

	Map<Locals> allLocals;
	Locals *locals;

	std::vector<Type> callArgTypes;
	unsigned blockDepth = 0;
	bool foundEarlyReturn = false;
};
