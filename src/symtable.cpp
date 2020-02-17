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
		if(!pushFunc(ptr->signature.name, &ptr->signature) ) {
			Global::errStack.push("Function redefinition '"
					+ ptr->signature.name + "'", ptr->token);
		}
		allLocals.insert(std::make_pair(
					ptr->signature.name,
					Locals() ) );
	}
	//Look ahead at all extern definitions
	for(auto ptr : node.externs) {
		if(!pushFunc(ptr->name, &ptr->signature) ) {
			Global::errStack.push("Function redefinition '"
					+ ptr->name + "'", ptr->token);
		}
	}
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(FunctionAstNode &node) {
	foundEarlyReturn = false;
	currentFunction = &node.signature;
	locals = &allLocals.find(node.signature.name)->second;
	for(size_t i = 0; i < node.signature.parameters.size(); i++) {
		locals->insert(std::make_pair(
			node.signature.paramNames[i],
			&node.signature.parameters[i]) );
	}

	//TODO: Trimming and analyzing the tree is probably not the SymTable's responsibility,
	//		Maybe refactor SymTable as a semantic analysis class with a symbol table as a
	//		biproduct?
	for(auto it = node.children.begin(); it != node.children.end(); it++) {
		if(foundEarlyReturn) {
			node.children.erase(it, node.children.end() );
			break;
		}
		(*it)->accept(*this);
	}

	//Check to see if a function returning data does not return any
	if(!foundEarlyReturn &&
	//TODO: Create type table to prevent these kind of necessities
			currentFunction->returnType != Type{"void", false} ) {
		Global::errStack.push("Function '" + node.signature.name 
				+ "' does not return a value, expected return of type '" 
				+ node.signature.returnType.string() + "'", node.token);
	}

	locals->clear();
}

void SymTable::visit(ExternAstNode &node) {
	//TODO: ?
}

void SymTable::visit(StatementAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}
	callArgTypes.clear();
}

void SymTable::visit(VariableDeclareAstNode &node) {
	//Check if type exists
	if(!hasType(node.type.name) ){
		Global::errStack.push("Type '" + node.type.name
				+ "' is not defined\n", node.token);
		return;
	}

	//Check function redefinition
	if(hasFunc(node.identifier) ) { 
		Global::errStack.push("Redefinition of identifier '" + node.identifier 
				+ "'", node.token);
		return;
	}

	//Check variable redefinition
	auto it = locals->find(node.identifier);
	if(it == locals->end() ) {
		locals->insert(std::make_pair(node.identifier, &node.type) );
	} else {
		Global::errStack.push("Redefinition of variable '"
				+ node.identifier + '\'', node.token);
	}
}

void SymTable::visit(ReturnAstNode &node) {
	callArgTypes.clear();
	for(const auto &child : node.children) {
		child->accept(*this);
	}

	for(auto &t : callArgTypes) {
		if(t != currentFunction->returnType) {
			Global::errStack.push("Function '" + currentFunction->name 
					+ "' tries to return value of type '" + t.string() 
					+ "', when definition specifies it to return '"
					+ currentFunction->returnType.string() + "'", node.token);
		}
	}
	callArgTypes.clear();
	foundEarlyReturn = true;
}

void SymTable::visit(CallAstNode &node) {
	auto sig = hasFunc(node.identifier);
	if(!sig) {
		Global::errStack.push("Function '" + node.identifier 
				+ "' does not exist", node.token);
		return;
	}

	auto oldTypes = std::move(callArgTypes);
	for(const auto &node : node.children) {
		node->accept(*this);
	}

	if(sig->parameters.size() != callArgTypes.size() || 
			!std::equal(sig->parameters.cbegin(), sig->parameters.cend(),
				callArgTypes.cbegin() ) ) {
		std::string errString = "Function call '" + node.identifier
				+ '(';
		for(int i = 0; i < callArgTypes.size(); i++) {
			errString += callArgTypes[i].string();
			if(i != callArgTypes.size() - 1) {
				errString += ", ";
			}
		}
		errString += ")' does not match function signature of '"
			+ node.identifier + '(';

		for(int i = 0; i < sig->parameters.size(); i++) {
			errString += sig->parameters[i].string();
			if(i != sig->parameters.size() - 1) {
				errString += ", ";
			}
		}

		errString += ")'";

		Global::errStack.push(errString, node.token);
	}

	//Clear this before next call is made
	oldTypes.push_back(functions[node.identifier]->returnType);
	callArgTypes = std::move(oldTypes);
}

void SymTable::visit(BinExpressionAstNode &node) {
	auto types = std::move(callArgTypes);
	for(const auto &child : node.children) {
		child->accept(*this);
	}

	if(callArgTypes.size() != 2) {
		return;
	}

	auto &lhs = callArgTypes.front();
	auto &rhs = callArgTypes.back();
	if(lhs != rhs) {
		Global::errStack.push(std::string("Type mismatch, cannot perform '") 
			+ Token::strings[static_cast<size_t>(node.type)].data()
			+ "' with '" + lhs.string() + "' and a '" + rhs.string() + '\'', node.token);
		types.push_back({"", false});
	} else {
		types.push_back(rhs);
	}
	callArgTypes = std::move(types);
}

void SymTable::visit(VariableAstNode &node) {
	auto it = locals->find(node.name);
	if(it == locals->end() ) {
		Global::errStack.push("Variable '"
				+ node.name + "' used but never defined", node.token);
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
