#include "ast.hpp"
#include "llvm.hpp"

void Ast::buildTree(Tokens &&tokens) {
	this->tokens = tokens;
	auto it = this->tokens.cbegin();

	expected = TokenType::Function;
	buildTree(it);
}

bool Ast::expect(TokenType type) {
	if(type != expected) {
		std::cerr << "Error! Expected " << tokenStrings[static_cast<size_t>(expected)]
			<< " recieved " << tokenStrings[static_cast<size_t>(type)] << '\n';
		return false;
	} else std::cerr << "Good\n";
	return true;
}

void Ast::buildTree(CTokenIterator it) {
	if(it == tokens.cend() ) return;

	if(expect(it->type) ) {
		switch(expected) {
			case TokenType::Function:
				expected = TokenType::Identifier;
				it = buildFunction(std::next(it) );
				break;
		}
	}

	if(it == tokens.cend() ) return;
	buildTree(std::next(it) );
}

CTokenIterator Ast::buildFunction(CTokenIterator it) {
	if(it == tokens.cend() ) return it;

	if(expect(it->type) ) {
		switch(expected) {
			case TokenType::Identifier:
				addChild(std::move(std::make_unique<FunctionAstNode>(it->value) ) );
				expected = TokenType::ParensOpen;
				break;
			case TokenType::ParensOpen:
				expected = TokenType::ParensClose;
				break;
			case TokenType::ParensClose:
				expected = TokenType::BlockOpen;
				break;
			case TokenType::BlockOpen:
				expected = TokenType::BlockClose;
				break;
			case TokenType::BlockClose:
				expected = TokenType::Function;
				return it;
				break;
		}
	}

	if(it == tokens.cend() ) return it;
	return buildFunction(std::next(it) );
}

void Ast::generateCode(Context &ctx, ModuleInfo &mi) {
	for(const auto &child : children) child->generateCode(ctx, mi);
}

FunctionAstNode::FunctionAstNode(const std::string &identifier) 
	: identifier(identifier) {
}

void FunctionAstNode::generateCode(Context &ctx, ModuleInfo &mi) {
	llvm::FunctionType *funcType = llvm::FunctionType::get(ctx.builder.getVoidTy(), false);
	llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, identifier, mi.module.get() );

	llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx.context, "entrypoint", mainFunc);
	ctx.builder.SetInsertPoint(entry);

	//Content goes here
	
	ctx.builder.CreateRetVoid();
}
