#include "ast.hpp"
#include "llvm.hpp"

void Ast::buildTree(Tokens &&tokens) {
	this->tokens = tokens;
	iterator = this->tokens.begin();

	buildTree();
}

bool Ast::generateCode(Context &ctx, ModuleInfo &mi) {
	for(const auto &child : children) {
		if(child) {
			child->generateCode(ctx, mi);
		} else return false;;
	}
	return true;
}

Token *Ast::getIf(TokenType type) {
	if(iterator == tokens.end() || iterator->type != type) return nullptr;
	return &*(iterator++);
}

void Ast::buildTree() {
	for(;;) {
		if(getIf(TokenType::Function) ) {
			addChild(std::move(buildFunction() ) );
		} else return;
	}
}

Ast::Child Ast::buildFunction() {
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

Ast::Child Ast::buildStatement() {
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

Ast::Child Ast::buildCall(const std::string &identifier) {
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

Ast::Child Ast::buildExpr() {
	//TODO: Expand this
	auto str = getIf(TokenType::StringLiteral);
	if(!str) return nullptr;
	return std::make_unique<StringAstNode>(str->value);
}

FunctionAstNode::FunctionAstNode(const std::string &identifier) 
	: identifier(identifier) {
}

bool FunctionAstNode::generateCode(Context &ctx, ModuleInfo &mi) {
	llvm::FunctionType *funcType = llvm::FunctionType::get(ctx.builder.getVoidTy(), false);
	llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, identifier, mi.module.get() );

	llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx.context, "entrypoint", mainFunc);
	ctx.builder.SetInsertPoint(entry);

	//Content goes here
	for(const auto &child : children) {
		if(!child->generateCode(ctx, mi) ) {
			return false;
		}
	}
	
	ctx.builder.CreateRetVoid();
	return true;
}

bool StatementAstNode::generateCode(Context &ctx, ModuleInfo &mi) {
	for(const auto &child : children) {
		if(!child->generateCode(ctx, mi) ) {
			return false;
		}
	}
	return true;
}

CallAstNode::CallAstNode(const std::string &identifier) 
	: identifier(identifier) {
}

bool CallAstNode::generateCode(Context &ctx, ModuleInfo &mi) {
	//TODO: Implement this
	return true;
}

bool ExpressionAstNode::generateCode(Context &ctx, ModuleInfo &mi) {
	for(const auto &child : children) {
		if(!child->generateCode(ctx, mi) ) {
			return false;
		}
	}
	return true;
}

StringAstNode::StringAstNode(const std::string &value) : value(value) {
}

bool StringAstNode::generateCode(Context &ctx, ModuleInfo &mi) {
	//TODO: Implement this
	return true;
}
