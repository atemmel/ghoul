#pragma once
#include "ast.hpp"

class AstPrinter : public AstVisitor {
public:
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
	void visit(ArrayAstNode &node) override;
	void visit(IndexAstNode &node) override;
	void visit(MemberVariableAstNode &node) override;
	void visit(VariableAstNode &node) override;
	void visit(StringAstNode &node) override;
	void visit(IntAstNode &node) override;
	void visit(BoolAstNode &node) override;

private:
	void pad(unsigned i) const;

	struct Scope {
		Scope();
		~Scope();
		static unsigned depth;
	};
};
