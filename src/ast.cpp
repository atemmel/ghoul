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

bool ToplevelAstNode::typeExists(const std::string &str) const {
	return types.count(str) > 0;
}

FunctionAstNode::FunctionAstNode(const std::string &identifier) 
	: identifier(identifier) {
}

void FunctionAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

ExternAstNode::ExternAstNode(const std::string &identifier)
	:identifier(identifier) {
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

void TypeAstNode::accept(AstVisitor &visitor) {
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
	toplevel->types.insert("void");
	toplevel->types.insert("char");
	toplevel->types.insert("int");
	toplevel->types.insert("float");

	root = toplevel.get();

	for(;;) {
		discardWhile(TokenType::Terminator);
		if(getIf(TokenType::Function) ) {
			auto func = buildFunction();
			if(!func) continue;
			//TODO: If func is empty, a parsing error has occured
			//Log this somehow for error messages
			auto fptr = static_cast<FunctionAstNode*>(func.get() );
			if(toplevel->identifiers.find(fptr->identifier)
					== toplevel->identifiers.end() ) {
				toplevel->identifiers.insert(fptr->identifier);
			} else {
				//TODO: Move to separate logging implementation
				unexpected();
				return nullptr;
			}
			toplevel->addFunction(fptr);
			toplevel->addChild(std::move(func) );
		} else if(getIf(TokenType::Extern) ) {
			auto ext = buildExtern();
			if(!ext) continue;
			toplevel->identifiers.insert(static_cast<ExternAstNode*>(ext.get() )->identifier );
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
	if(!token) return nullptr;

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

	while(!getIf(TokenType::BlockClose) ) {
		discardWhile(TokenType::Terminator);
		auto stmnt = buildStatement();
		if(stmnt) function->addChild(std::move(stmnt) );
		else {
			Global::errStack.push("Could not build valid statement", *iterator);
			return nullptr;
		}
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
	for(auto child = buildType(); child;) {
		ext->addChild(std::move(child) );
		getIf(TokenType::Identifier);
	}

	if(!getIf(TokenType::ParensClose) ) {
		return unexpected();
	}

	auto result = buildType();
	if(result) {
		ext->addChild(std::move(result) );
	} else {
		//TODO: Shorten this a bit by adding a ctor to TypeAstNode
		auto voidType = std::make_unique<TypeAstNode>();
		voidType->name = "void";
		ext->addChild(std::move(voidType) );
	}

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
			return nullptr;
		}
		stmnt->addChild(std::move(call) );
		return stmnt;
	}
	return nullptr;
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
	if(!str) return unexpected();
	return std::make_unique<StringAstNode>(str->value);
}

AstNode::Child AstParser::buildType() {
	auto id = getIf(TokenType::Identifier);
	if(!id) {
		return unexpected();
	}
	if(!root->typeExists(id->value) ) {
		//TODO: This isn't really unexpected, but rather a consequence of types not existing
		return unexpected();
	}
	auto type = std::make_unique<TypeAstNode>();
	type->name = id->value;
	type->isPtr = getIf(TokenType::And) != nullptr;
	return type;
}
