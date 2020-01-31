#include "ast.hpp"
#include "llvm.hpp"

AstNode::Root AstParser::buildTree(Tokens &&tokens) {
	this->tokens = tokens;
	iterator = this->tokens.begin();
	return buildTree();
}

Token *AstParser::getIf(TokenType type) {
	if(iterator == tokens.end() || iterator->type != type) return nullptr;
	return &*(iterator++);
}

AstNode::Root AstParser::buildTree() {
	auto toplevel = std::make_unique<ToplevelAstNode>();
	for(;;) {
		if(getIf(TokenType::Function) ) {
			toplevel->addChild(std::move(buildFunction() ) );
		} else {
			break;
		}
	}
	return toplevel;
}

AstNode::Child AstParser::buildFunction() {
	Token *token = getIf(TokenType::Identifier);
	if(!token) return nullptr;

	auto function = std::make_unique<FunctionAstNode>(token->value);

	//TODO: Expand on this to support parameters
	if(!getIf(TokenType::ParensOpen) ) {
		return nullptr;
	}

	if(!getIf(TokenType::ParensClose) ) {
		return nullptr;
	}
	
	if(!getIf(TokenType::BlockOpen) ) {
		return nullptr;
	}

	while(!getIf(TokenType::BlockClose) ) {
		auto stmnt = buildStatement();
		if(!stmnt) return nullptr;
		function->addChild(std::move(stmnt) );
	}

	return function;
}

AstNode::Child AstParser::buildStatement() {
	auto stmnt = std::make_unique<StatementAstNode>();
	Token *token = getIf(TokenType::Identifier);
	if(getIf(TokenType::ParensOpen) ) {
		auto call = buildCall(token->value);
		if(!call) return nullptr;
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
		return nullptr;
	}
	return call;
}

AstNode::Child AstParser::buildExpr() {
	//TODO: Expand this
	auto str = getIf(TokenType::StringLiteral);
	if(!str) return nullptr;
	return std::make_unique<StringAstNode>(str->value);
}

void ToplevelAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

FunctionAstNode::FunctionAstNode(const std::string &identifier) 
	: identifier(identifier) {
}

void FunctionAstNode::accept(AstVisitor &visitor) {
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
