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
		if(!pushFunc(ptr->name, &ptr->signature) ) {
			Global::errStack.push("Function redefinition \""
					+ ptr->name + "\"", Token() );
		}
	}
	//Look ahead at all extern definitions
	for(auto ptr : node.externs) {
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
		return;
	}

	for(const auto &node : node.children) {
		node->accept(*this);
	}

	if(sig->parameters.size() != callArgTypes.size() || 
			!std::equal(sig->parameters.cbegin(), sig->parameters.cend(),
				callArgTypes.cbegin() ) ) {
		std::string errString = "Function call \"" + node.identifier
				+ '(';
		for(int i = 0; i < callArgTypes.size(); i++) {
			errString += callArgTypes[i].name;
			if(callArgTypes[i].isPtr) errString.push_back('&');
			if(i != callArgTypes.size() - 1) {
				errString += ", ";
			}
		}
		errString += ")\" does not match function signature of \""
			+ node.identifier + '(';

		for(int i = 0; i < sig->parameters.size(); i++) {
			errString += sig->parameters[i].name;
			if(sig->parameters[i].isPtr) errString.push_back('&');
			if(i != sig->parameters.size() - 1) {
				errString += ", ";
			}
		}

		errString += ")\"";

		Global::errStack.push(errString, Token() );
	}

	//Clear this before next call is made
	callArgTypes.clear();
}

void SymTable::visit(ExpressionAstNode &node) {
	callArgTypes.push_back(node.type);
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(StringAstNode &node) {

}
