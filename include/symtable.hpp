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
	void visit(FunctionAstNode &node) override;
	void visit(ExternAstNode &node) override;
	void visit(StatementAstNode &node) override;
	void visit(CallAstNode &node) override;
	void visit(ExpressionAstNode &node) override;
	void visit(StringAstNode &node) override;

private:
	template <typename T>
	using Map = std::unordered_map<std::string, T>;
	using Set = std::unordered_set<std::string>;

	Set types;
	Map<const FunctionSignature*> functions;
};
