#pragma once
#include "ast.hpp"
#include "global.hpp"

#include <unordered_map>
#include <unordered_set>

class SymTable : public AstVisitor {
public:
	SymTable();

	bool pushFunc(const std::string &identifier, FunctionSignature *func) {
		return functions.insert({identifier, func}).second;
	}

	const FunctionSignature *hasFunc(const std::string &identifier) const;

	bool hasType(const std::string &identifier) const;

	void visit(ToplevelAstNode &node) override;
	void visit(StructAstNode &node) override;
	void visit(FunctionAstNode &node) override;
	void visit(ExternAstNode &node) override;
	void visit(StatementAstNode &node) override;
	void visit(VariableDeclareAstNode &node) override;
	void visit(ReturnAstNode &node) override;
	void visit(CallAstNode &node) override;
	void visit(BinExpressionAstNode &node) override;
	void visit(VariableAstNode &node) override;
	void visit(StringAstNode &node) override;
	void visit(IntAstNode &node) override;

private:
	template <typename T>
	using Map = std::unordered_map<std::string, T>;
	using Set = std::unordered_set<std::string>;
	using Locals = Map<const Type*>;

	Set types;
	Map<const FunctionSignature*> functions;
	const FunctionSignature *currentFunction = nullptr;

	Map<Locals> allLocals;
	Locals *locals;

	std::vector<Type> callArgTypes;
	bool foundEarlyReturn = false;
};
