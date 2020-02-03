#include "astprint.hpp"

#include <iostream>

unsigned AstPrinter::Scope::depth = 0;

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

void AstPrinter::visit(ExternAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Extern : " << node.identifier << '\n';
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

void AstPrinter::visit(TypeAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Type : " << node.name << (node.isPtr ? " &" : "") << '\n';
}

void AstPrinter::visit(StringAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "String : " << node.value << '\n';
}

void AstPrinter::pad(unsigned i) const {
	for(unsigned j = 1; j < i; j++) {
		std::cerr << "  ";
	}
}

AstPrinter::Scope::Scope() {
	depth++;
}

AstPrinter::Scope::~Scope() {
	depth--;
}
