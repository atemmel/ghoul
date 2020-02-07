#include "ast.hpp"
#include "global.hpp"
#include "llvm.hpp"

void AstNode::addChild(Child && child) {
	children.push_back(std::move(child) );
}

void ToplevelAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void ToplevelAstNode::addFunction(FunctionAstNode *func) {
	functions.push_back(func);
}

void ToplevelAstNode::addExtern(ExternAstNode *ext) {
	externs.push_back(ext);
}

FunctionAstNode::FunctionAstNode(const std::string &identifier) 
	: name(identifier) {
}

void FunctionAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

ExternAstNode::ExternAstNode(const std::string &identifier)
	: name(identifier) {
}

void ExternAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void StatementAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

CallAstNode::CallAstNode(const std::string &identifier) 
	: identifier(identifier) {
}

void CallAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void ExpressionAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

StringAstNode::StringAstNode(const std::string &value) : value(value) {
}

void StringAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

AstNode::Root AstParser::buildTree(Tokens &&tokens) {
	this->tokens = tokens;
	iterator = this->tokens.begin();
	return buildTree();
}

AstNode::Child AstParser::unexpected() const {
	Global::errStack.push("Unexpected token: \"" + iterator->value + '\"', *iterator);
	return nullptr;
}

Token *AstParser::getIf(TokenType type) {
	if(iterator == tokens.end() || iterator->type != type) return nullptr;
	return &*(iterator++);
}

void AstParser::discardWhile(TokenType type) {
	while(getIf(type) );
}

AstNode::Root AstParser::buildTree() {
	auto toplevel = std::make_unique<ToplevelAstNode>();

	root = toplevel.get();

	for(;;) {
		discardWhile(TokenType::Terminator);
		if(getIf(TokenType::Function) ) {
			auto func = buildFunction();
			if(!func) continue;
			//TODO: If func is empty, a parsing error has occured
			//Log this somehow for error messages
			auto fptr = static_cast<FunctionAstNode*>(func.get() );
			toplevel->addFunction(fptr);
			toplevel->addChild(std::move(func) );
		} else if(getIf(TokenType::Extern) ) {
			auto ext = buildExtern();
			if(!ext) continue;
			auto eptr = static_cast<ExternAstNode*>(ext.get() );
			toplevel->addExtern(eptr);
			toplevel->addChild(std::move(ext) );
		} else if(iterator == tokens.end() ) {
			break;
		} else {
			unexpected();
			return nullptr;
		}
	}
	root = nullptr;
	if(toplevel->children.empty() ) {
		return nullptr;
	}
	return toplevel;
}

AstNode::Child AstParser::buildFunction() {
	Token *token = getIf(TokenType::Identifier);

	if(!token) return unexpected();

	auto function = std::make_unique<FunctionAstNode>(token->value);

	//TODO: Expand on this to support parameters
	if(!getIf(TokenType::ParensOpen) ) {
		return unexpected();
	}

	if(!getIf(TokenType::ParensClose) ) {
		return unexpected();
	}
	
	if(!getIf(TokenType::BlockOpen) ) {
		return unexpected();
	}

	//TODO: This is yucky
	function->signature.returnType = {"void", false };

	discardWhile(TokenType::Terminator);
	while(!getIf(TokenType::BlockClose) ) {
		auto stmnt = buildStatement();
		if(stmnt) function->addChild(std::move(stmnt) );
		else {
			Global::errStack.push("Could not build valid statement", *iterator);
			return nullptr;
		}
		discardWhile(TokenType::Terminator);
	}

	return function;
}

AstNode::Child AstParser::buildExtern() {
	if(!getIf(TokenType::Function) ) {
		return unexpected();
	}

	auto id = getIf(TokenType::Identifier);
	if(!id) {
		return unexpected();
	}

	if(!getIf(TokenType::ParensOpen) ) {
		return unexpected();
	}

	auto ext = std::make_unique<ExternAstNode>(id->value);
	while(!getIf(TokenType::ParensClose) ) {
		id = getIf(TokenType::Identifier);
		if(!id) {
			return unexpected();
		}
		Type type;
		type.name = id->value;
		type.isPtr = getIf(TokenType::And);
		getIf(TokenType::Identifier);
		ext->signature.parameters.push_back(type);
		//TODO: Comma?
	}

	Type result;
	id = getIf(TokenType::Identifier);
	if(id) {
		result.name = id->value;
		result.isPtr = getIf(TokenType::And);
	} else {
		result.name = "void";
	}

	ext->signature.returnType = result;

	if(!getIf(TokenType::Terminator) ) {
		return unexpected();
	}

	return ext;
}

AstNode::Child AstParser::buildStatement() {
	auto stmnt = std::make_unique<StatementAstNode>();
	Token *token = getIf(TokenType::Identifier);
	if(getIf(TokenType::ParensOpen) ) {
		auto call = buildCall(token->value);
		if(!call) {
			return unexpected();
		}
		stmnt->addChild(std::move(call) );
		return stmnt;
	}
	return unexpected();
}

AstNode::Child AstParser::buildCall(const std::string &identifier) {
	auto call = std::make_unique<CallAstNode>(identifier);
	auto expr = buildExpr();	//TODO: Expand on this to allow for multiple parameters
	if(expr) {
		call->addChild(std::move(expr) );
	}
	if(!getIf(TokenType::ParensClose) ) {
		return unexpected();
	}
	if(!getIf(TokenType::Terminator) ) {
		std::cerr << "Expected end of expression\n";
		std::cerr << static_cast<size_t>(iterator->type) << " : " << iterator->value << '\n';
		return unexpected();
	}
	return call;
}

AstNode::Child AstParser::buildExpr() {
	//TODO: Expand this
	auto str = getIf(TokenType::StringLiteral);
	if(!str) return nullptr;
	return std::make_unique<StringAstNode>(str->value);
}

/*
Type *AstParser::buildType() {
	auto id = getIf(TokenType::Identifier);
	if(!id) {
		return unexpected();
	}
	//TODO: Ugh
	/*
	if(!root->typeExists(id->value) ) {
		//TODO: This isn't really unexpected, but rather a consequence of types not existing
		return unexpected();
	}
	auto type = std::make_unique<TypeAstNode>();
	type->name = id->value;
	type->isPtr = getIf(TokenType::And) != nullptr;
	return type;
}
*/
