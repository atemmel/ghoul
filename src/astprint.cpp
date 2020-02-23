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

void AstPrinter::visit(StructAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Struct : " << node.name << '\n';
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(FunctionAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Function : " << node.signature.name << '\n';
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(ExternAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Extern : " << node.name << '\n';
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

void AstPrinter::visit(VariableDeclareAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Decl : " << node.identifier << " as " 
		<< node.type.name << (node.type.isPtr ? "&\n" : "\n");
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(ReturnAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Return\n";
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

void AstPrinter::visit(BinExpressionAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Binary Expression : " 
		<< Token::strings[static_cast<size_t>(node.type)] << '\n';
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void AstPrinter::visit(MemberVariableAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Member accessed : " << node.name << '\n';
}

void AstPrinter::visit(VariableAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Variable : " << node.name << '\n';
}

void AstPrinter::visit(StringAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "String : '" << node.value << "'\n";
}

void AstPrinter::visit(IntAstNode &node) {
	Scope scope;
	pad(scope.depth);
	std::cerr << "Int : " << node.value << '\n';
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
