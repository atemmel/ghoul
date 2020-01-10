#include "ast.hpp"
#include "llvm.hpp"

void Ast::buildTree(const Tokens &tokens) {
	auto it = tokens.begin();
	auto end = tokens.end();

	while(it != end) {
		switch(it->type) {
			case TokenType::Function: {
				it = buildFunction(it, end);
				break;
			}
		}
	}
}

CTokenIterator Ast::buildFunction(CTokenIterator first, CTokenIterator last) {
	auto next = std::next(first);
	if(next->type != TokenType::Identifier) {
		//TODO: Error!
		return last;
	}

	//TODO: Generalize this
	auto lparen = std::next(next);
	auto rparen = std::next(lparen);
	
	if(lparen->type != TokenType::ParensOpen 
			|| rparen->type != TokenType::ParensClose) {
		return last;
	}

	auto node = std::make_unique<FunctionAstNode>(next->value);
	if(!root) {	//TODO: Identify and separate main
		root = std::move(node);
	}
	else {
		//root->addChild(std::move(node) );
	}

	//TODO: Generalize this
	auto lcurly = std::next(rparen);
	auto rcurly = std::next(lcurly);

	if(lcurly->type != TokenType::BlockOpen
			|| rcurly->type != TokenType::BlockClose) {
		return last;
	}

	return std::next(rcurly);
}

void Ast::generateCode(Context &ctx, ModuleInfo &mi) {
	root->generateCode(ctx, mi);
}

void FunctionAstNode::generateCode(Context &ctx, ModuleInfo &mi) {
	llvm::FunctionType *funcType = llvm::FunctionType::get(ctx.builder.getVoidTy(), false);
	llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, value, mi.module.get() );

	llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx.context, "entrypoint", mainFunc);
	ctx.builder.SetInsertPoint(entry);

	//Content goes here
	
	ctx.builder.CreateRetVoid();
}
