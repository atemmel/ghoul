#pragma once
#include "ast.hpp"

class AstPrinter : public AstVisitor {
public:
	void visit(ToplevelAstNode &node) override;
	void visit(FunctionAstNode &node) override;
	void visit(StatementAstNode &node) override;
	void visit(CallAstNode &node) override;
	void visit(ExpressionAstNode &node) override;
	void visit(StringAstNode &node) override;
private:
	void pad(unsigned i) const;
	struct Scope {
		Scope();
		~Scope();
		static unsigned depth;
	};
};
