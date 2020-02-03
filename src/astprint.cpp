#include "astprint.hpp"

#include <iostream>

struct Scope {
	Scope() {
		depth++;
	}

	~Scope() {
		depth--;
	}

	static unsigned depth;
};

unsigned Scope::depth = 0;

void pad(unsigned i) {
	for(unsigned j = 1; j < i; j++) {
		std::cerr << "  ";
	}
}

void AstPrinter::visit(ToplevelAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Toplevel\n";
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(FunctionAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Function : " << node.identifier << '\n';
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(StatementAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Statement\n";
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(CallAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Call : " << node.identifier << '\n';
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(ExpressionAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Expression\n";
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(StringAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "String : " << node.value << '\n';
}
