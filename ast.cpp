#include "ast.hpp"
#include "llvm.hpp"

void Ast::buildTree(const Tokens &tokens) {
	auto it = tokens.cbegin();
	auto end = tokens.cend();

	auto buildFunction = [&]() {
		auto next = std::next(it);
		if(next->type != TokenType::Identifier) {
			//TODO: Error!
			return;
		}

		//TODO: Generalize this
		auto lparen = std::next(next);
		auto rparen = std::next(lparen);
		
		if(lparen->type != TokenType::ParensOpen 
				|| rparen->type != TokenType::ParensClose) {
			return;
		}

		root = std::make_unique<FunctionAstNode>(next->value);

		//TODO: Generalize this
		auto lcurly = std::next(rparen);
		auto rcurly = std::next(lcurly);

		if(lcurly->type != TokenType::BlockOpen
				|| rcurly->type != TokenType::BlockClose) {
			return;
		}

		it = std::next(rcurly);
	};

	switch(it->type) {
		case TokenType::Function: {
			buildFunction();
			break;
		}
	}
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
