#include "symtable.hpp"

//Default types
SymTable::SymTable() {
	types.insert("void");
	types.insert("char");
	types.insert("int");
	types.insert("float");
}

const FunctionSignature *SymTable::hasFunc(const std::string &identifier) const {
	auto it = functions.find(identifier);
	if(it == functions.end() ) {
		return nullptr;
	}
	return it->second;
}

bool SymTable::hasType(const std::string &identifier) const {
	auto it = types.find(identifier);
	return it != types.end();
}

void SymTable::visit(ToplevelAstNode &node) {
	//Look ahead at all function definitions
	for(auto ptr : node.functions) {
		std::cout << "Preevaling " << ptr->name << '\n';
		if(!pushFunc(ptr->name, &ptr->signature) ) {
			Global::errStack.push("Function redefinition \""
					+ ptr->name + "\"", Token() );
		}
	}
	//Look ahead at all extern definitions
	for(auto ptr : node.externs) {
		std::cout << "Preevaling " << ptr->name << '\n';
		if(!pushFunc(ptr->name, &ptr->signature) ) {
			Global::errStack.push("Function redefinition \""
					+ ptr->name + "\"", Token() );
		}
	}
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(FunctionAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(ExternAstNode &node) {
	//TODO: ?
}

void SymTable::visit(StatementAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(CallAstNode &node) {
	auto sig = hasFunc(node.identifier);
	if(!sig) {
		Global::errStack.push("Function \"" + node.identifier 
				+ "\" does not exist", Token() );
	}
	//TODO: visit expr subnodes and match their types
}

void SymTable::visit(ExpressionAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(StringAstNode &node) {

}
