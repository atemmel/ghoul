#include "symtable.hpp"

//Default types
SymTable::SymTable() {
	types.insert("void");
	types.insert("char");
	types.insert("int");
	types.insert("float");
	types.insert("...");
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
					+ ptr->name + "\"", ptr->token);
		}
	}
	//Look ahead at all extern definitions
	for(auto ptr : node.externs) {
		if(!pushFunc(ptr->name, &ptr->signature) ) {
			Global::errStack.push("Function redefinition \""
					+ ptr->name + "\"", ptr->token);
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

void SymTable::visit(VariableDeclareAstNode &node) {
	//Check if type exists
	if(!hasType(node.type.name) ){
		Global::errStack.push("Type \"" + node.type.name
				+ "\" is not defined\n", node.token);
		return;
	}

	//Check function redefinition
	if(hasFunc(node.identifier) ) { 
		Global::errStack.push("Redefinition of identifier \"" + node.identifier 
				+ "\"", node.token);
		return;
	}

	//Check variable redefinition
	auto it = locals.find(node.identifier);
	if(it == locals.end() ) {
		locals.insert(std::make_pair(node.identifier, &node.type) );
	} else {
		Global::errStack.push("Redefinition of variable \""
				+ node.identifier + '\"', node.token);
	}
}

void SymTable::visit(CallAstNode &node) {
	auto sig = hasFunc(node.identifier);
	if(!sig) {
		Global::errStack.push("Function \"" + node.identifier 
				+ "\" does not exist", node.token);
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
			if(callArgTypes[i].isPtr) errString.push_back('*');
			if(i != callArgTypes.size() - 1) {
				errString += ", ";
			}
		}
		errString += ")\" does not match function signature of \""
			+ node.identifier + '(';

		for(int i = 0; i < sig->parameters.size(); i++) {
			errString += sig->parameters[i].name;
			if(sig->parameters[i].isPtr) errString.push_back('*');
			if(i != sig->parameters.size() - 1) {
				errString += ", ";
			}
		}

		errString += ")\"";

		Global::errStack.push(errString, node.token);
	}

	//Clear this before next call is made
	callArgTypes.clear();
}

void SymTable::visit(ExpressionAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}
void SymTable::visit(BinExpressionAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(VariableAstNode &node) {
	auto it = locals.find(node.name);
	if(it == locals.end() ) {
		Global::errStack.push("Variable \""
				+ node.name + "\" used but never defined", node.token);
	} else {
		callArgTypes.push_back(*it->second);
	}
}

void SymTable::visit(StringAstNode &node) {
	callArgTypes.push_back({
		"char",
		true
	});
}

void SymTable::visit(IntAstNode &node) {
	callArgTypes.push_back({
		"int",
		false
	});
	
}
