#include "symtable.hpp"

//Default types
SymTable::SymTable() {
	structs.insert({"void" , {"void"}  });
	structs.insert({"char" , {"char"}  });
	structs.insert({"int"  , {"int"}   });
	structs.insert({"float", {"float"} });
	structs.insert({"bool" , {"bool"}  });
}

void SymTable::dump() const {
	std::cerr << "Types:\n";
	for(const auto &type : structs) {
		std:: cerr << type.second.fullString() << '\n';
	}

	std::cerr << "Functions:\n";
	for(const auto &pair : functions) {
		std::cerr << pair.first << '\n';
		std::cerr << '\t' << "returns " << pair.second->returnType.string() << '\n';
		for(size_t i = 0; i < pair.second->parameters.size(); i++) {
			std::cerr << '\t' << pair.second->parameters[i].string() << ' ' 
				<< pair.second->paramNames[i] << '\n';
		}
		std::cerr << '\n';
	}
}

bool SymTable::pushFunc(const std::string &identifier, FunctionSignature *func) {
	return functions.insert({identifier, func}).second;
}

void SymTable::setActiveFunction(const std::string &str) {
	locals = &allLocals[str];
}

const Type *SymTable::getLocal(const std::string &str) const {
	return (*locals)[str].type;
}

const FunctionSignature *SymTable::hasFunc(const std::string &identifier) const {
	auto it = functions.find(identifier);
	if(it == functions.end() ) {
		return nullptr;
	}
	return it->second;
}

const Type* SymTable::hasStruct(const std::string &identifier) const {
	auto it = structs.find(identifier);
	if(it == structs.end() ) {
		return nullptr;
	}
	return &it->second;
}

const Type *SymTable::typeHasMember(const Type &type, const std::string &identifier) const {
	auto it = structs.find(type.name);
	if(it == structs.end() ) {
		return nullptr;
	}

	auto &members = it->second.members;
	auto jt = std::find_if(members.begin(), members.end(), [&](const Member &member) {
		return member.identifier == identifier;
	});

	if(jt == members.end() ) {
		return nullptr;
	}

	return &jt->type;
}

unsigned SymTable::getMemberOffset(const Type &type, const std::string &identifier) const {
	auto it = structs.find(type.name);
	if(it == structs.end() ) {
		//TODO: Hmmm
		return -1;
	}

	auto &members = it->second.members;
	unsigned i = 0;
	for(; i < members.size(); i++) {
		if(identifier == members[i].identifier) {
			return i;
		}
	}
	return i;
}

void SymTable::visit(ToplevelAstNode &node) {
	if(node.analyzed) {	//No need to do it again
		return;
	}

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

	for(auto ptr : node.structs) {
		if(!hasStruct(ptr->name) ) {
			structs.insert({ptr->name, {ptr->name} });
		} else {
			Global::errStack.push("Type redefinition '"
					+ ptr->name + "'", ptr->token);
		}
	}

	for(auto ptr : node.structs) {
		ptr->accept(*this);
	}

	for(const auto &child : node.children) {
		child->accept(*this);
	}

	node.analyzed = true;
}

void SymTable::visit(LinkAstNode &node) {

}

void SymTable::visit(StructAstNode &node) {
	Type type;
	type.name = node.name;
	Locals structMembers;
	locals = &structMembers;

	for(const auto &child : node.children) {
		child->accept(*this);
	}

	locals = nullptr;

	Member member;
	for(const auto &visited : structMembers) {
		member.identifier = visited.first;
		member.type = *visited.second.type;
		type.members.push_back(member);
	}

	structs[node.name] = type;
}

void SymTable::visit(FunctionAstNode &node) {
	foundEarlyReturn = false;
	currentFunction = &node.signature;
	locals = &allLocals.find(node.signature.name)->second;
	for(size_t i = 0; i < node.signature.parameters.size(); i++) {
		locals->insert(std::make_pair(
			node.signature.paramNames[i],
			Local{&node.signature.parameters[i], blockDepth}) );
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
		callArgTypes.clear();
	}

	//Check to see if a function returning data does not return any
	if(!foundEarlyReturn &&
	//TODO: Create type table to prevent these kind of necessities
			currentFunction->returnType != Type{"void", false} ) {
		Global::errStack.push("Function '" + node.signature.name 
				+ "' does not return a value, expected return of type '" 
				+ node.signature.returnType.string() + "'", node.token);
	}

	//locals->clear();
}

void SymTable::visit(ExternAstNode &node) {
	//TODO: ?
}

void SymTable::visit(VariableDeclareAstNode &node) {
	//Check if type exists
	if(!hasStruct(node.type.name) && !node.type.name.empty() ){
		Global::errStack.push("Type '" + node.type.name
				+ "' is not defined\n", node.token);
		return;
	}

	if(node.type.name == "void" && node.type.isPtr == 0) { 
		Global::errStack.push("May not declare variable of type 'void'", node.token);
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
		locals->insert(std::make_pair(node.identifier, Local{&node.type, blockDepth}) );
		if(node.type.name.empty() ) {	//var case

			//First child is binary expr (assignment), assignments rhs is expected type
			node.children.front()->children.back()->accept(*this);
			node.type = callArgTypes.front();
			callArgTypes.clear();
		}
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
	if(blockDepth == 0) {
		foundEarlyReturn = true;
	}
}

void SymTable::visit(BranchAstNode &node) {
	if(!demoteExprToBool(node.expr) ) {
		return;
	}

	blockDepth++;

	//TODO: LOCALS?????
	for(const auto &child : node.children) {
		child->accept(*this);
	}

	blockDepth--;
}

void SymTable::visit(LoopAstNode &node) {
	blockDepth++;
	if(node.loopPrefix) {
		node.loopPrefix->accept(*this);
		callArgTypes.clear();
	}

	node.expr->accept(*this);
	callArgTypes.clear();
	if(!demoteExprToBool(node.expr) ) {
		blockDepth--;
		return;
	}
	callArgTypes.clear();

	if(node.loopSuffix) {
		node.loopSuffix->accept(*this);
		callArgTypes.clear();
	}



	//TODO: LOCALS?????
	for(const auto &child : node.children) {
		child->accept(*this);
	}

	blockDepth--;
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

	auto matches = [](const std::vector<Type> &sig, const std::vector<Type> &args) {
		size_t overlap = 0;
		Type voidPtrTy = {"void", 1, false};
		for(auto sigit = sig.cbegin(), argsit = args.cbegin(); 
				sigit != sig.cend() && argsit != args.cend(); sigit++, argsit++, overlap++) {
			if(*sigit == voidPtrTy && (argsit->isPtr > 0 || argsit->isArray) ) {
				continue;
			}

			if(*sigit != *argsit) {
				break;
			}
		}

		if(overlap == sig.size() && overlap == args.size() ) {
			return true;
		}

		if(overlap == sig.size() - 1 && !sig.empty() && sig.back().name == "...") {
			return true;
		}

		return false;
	};

	if(!matches(sig->parameters, callArgTypes) ) {
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
		switch(node.type) {
			case TokenType::Add:
			case TokenType::Multiply:
			case TokenType::Divide:
			case TokenType::Subtract:
			case TokenType::Assign:
				types.push_back(rhs);
				break;
			case TokenType::Equivalence:
			case TokenType::NotEquivalence:
			case TokenType::Greater:
			case TokenType::GreaterEquals:
			case TokenType::Less:
			case TokenType::LessEquals:
				types.push_back({"bool", false});
				break;
			default:
				Global::errStack.push("Missing case for binary operator '" + node.token->value
					+ "'", node.token);
				return;
		}
	}
	callArgTypes = std::move(types);
}

void SymTable::visit(UnaryExpressionAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}

	if(node.type == TokenType::And) {
		if(callArgTypes.back().isPtr < 1) {
			Global::errStack.push("Cannot dereference further", node.token);
			return;
		}
		callArgTypes.back().isPtr--;
	} else if(node.type == TokenType::Multiply) { 
		callArgTypes.back().isPtr++;
	} else if(node.type == TokenType::Ternary) {
		callArgTypes.back() = {"int", 0, false};
	}
}

void SymTable::visit(CastExpressionAstNode &node) {
	for(const auto &child : node.children) {
		child->accept(*this);
	}

	if(!hasStruct(node.type.name) ) {
		Global::errStack.push("Cannot cast '" + callArgTypes.back().name
				+ "' into '" + node.type.name + "'", node.token);
		callArgTypes.clear();
		return;
	}

	//I mean...
	callArgTypes.back() = node.type;
}

void SymTable::visit(ArrayAstNode &node) {
	if(node.length) {
		node.length->accept(*this);
		if(callArgTypes.back() != Type{"int", 0, false} ) {
			Global::errStack.push("Array declaration expects length definition to be of type 'int'", 
				node.length->token);
		}
		callArgTypes.back() = node.type;
	} else {
		callArgTypes.push_back(node.type);
	}
}

void SymTable::visit(IndexAstNode &node) {
	if(!callArgTypes.back().isArray) {
		Global::errStack.push("Cannot index into type '" + callArgTypes.back().string() + "'", node.token);
	}

	node.index->accept(*this);
	if(callArgTypes.back() != Type{"int", 0, false} ) {
		Global::errStack.push("Indexing a variable requires the index to be of type 'int'",
			node.index->token);
	}
	callArgTypes.pop_back();
	callArgTypes.back().isArray = false;
}

void SymTable::visit(MemberVariableAstNode &node) {
	auto &back = callArgTypes.back();
	auto member = typeHasMember(back, node.name);
	if(!member) {
		Global::errStack.push(std::string("Type '" + back.string() + "' has no member '"
			+ node.name + "'"), node.token);
		callArgTypes.clear();
		//TODO: Is this a good idea?
		//callArgTypes.push_back({"", false});
		return;
	}

	callArgTypes.pop_back();
	callArgTypes.push_back(*member);

	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void SymTable::visit(VariableAstNode &node) {
	auto it = locals->find(node.name);
	if(it == locals->end() || it->second.depth > blockDepth) {
		Global::errStack.push("Variable '"
			+ node.name + "' used but never defined", node.token);
	} else {
		callArgTypes.push_back(*it->second.type);
		for(const auto &child : node.children) {
			child->accept(*this);
		}
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

void SymTable::visit(BoolAstNode &node) {
	callArgTypes.push_back({
		"bool",
		false
	});
}

bool SymTable::demoteExprToBool(AstNode::Expr &expr) {
	expr->accept(*this);
	Type &result = callArgTypes.back();
	if(result == Type{"int", false} || result.isPtr > 0) { //Compare numeric values to zero		
		auto binop = std::make_unique<BinExpressionAstNode>(TokenType::NotEquivalence);
		binop->addChild(std::move(expr) );
		binop->addChild(std::make_unique<IntAstNode>(0) );
		expr = std::move(binop);
	} else if(result != Type{"bool", false}) {	//If non bool expr
		Global::errStack.push("Cannot translate result of expression into type'bool'", 
				expr->token);
		callArgTypes.clear();
		return false;
	}

	callArgTypes.clear();
	return true;
}
