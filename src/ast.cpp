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

FunctionAstNode::FunctionAstNode(const std::string &identifier) {
	signature.name = identifier;
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

void VariableDeclareAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void ReturnAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

CallAstNode::CallAstNode(const std::string &identifier) 
	: identifier(identifier) {
}

void CallAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

BinExpressionAstNode::BinExpressionAstNode(TokenType type) 
	: type(type) {
	precedence = Token::precedence(type);
}

void BinExpressionAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

VariableAstNode::VariableAstNode(const std::string &name)
	: name(name) {
	precedence = Token::precedence(TokenType::Identifier);
}

void VariableAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

StringAstNode::StringAstNode(const std::string &value) : value(value) {
	precedence = Token::precedence(TokenType::StringLiteral);
}

void StringAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

IntAstNode::IntAstNode(const std::string &value) {
	isIntLiteral(value, this->value);
	precedence = Token::precedence(TokenType::IntLiteral);
}

void IntAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

AstNode::Root AstParser::buildTree(Tokens &&tokens) {
	this->tokens = tokens;
	iterator = this->tokens.begin();
	return buildTree();
}

AstNode::Child AstParser::unexpected() const {
	Global::errStack.push("Unexpected token: '" + iterator->value + '\'', &*iterator);
	return nullptr;
}

Token *AstParser::getIf(TokenType type) {
	if(iterator == tokens.end() || iterator->type != type) return nullptr;
	return &*(iterator++);
}

void AstParser::unget() {
	if(iterator != tokens.begin() ) {
		--iterator;
	}
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
	function->token = token;

	if(!getIf(TokenType::ParensOpen) ) {
		return unexpected();
	}

	for(;;) {
		if(getIf(TokenType::ParensClose) ) {
			break;
		}

		Type type;
		auto typeId = getIf(TokenType::Identifier);
		if(!typeId) {
			return unexpected();
		}

		type.name = typeId->value;
		type.isPtr = getIf(TokenType::Multiply);
		
		auto parId = getIf(TokenType::Identifier);
		if(!parId) {
			return unexpected();
		}

		function->signature.parameters.push_back(type);
		function->signature.paramNames.push_back(parId->value);

		if(getIf(TokenType::ParensClose) ) {
			break;
		} else if(!getIf(TokenType::Comma) ) {
			return unexpected();
		}
	}

	auto ret = getIf(TokenType::Identifier);
	if(ret) {
		function->signature.returnType = {ret->value, getIf(TokenType::Multiply) };
	} else {
		function->signature.returnType = {"void", false };
	}
	
	if(!getIf(TokenType::BlockOpen) ) {
		return unexpected();
	}

	discardWhile(TokenType::Terminator);
	while(!getIf(TokenType::BlockClose) ) {
		auto stmnt = buildStatement();
		if(stmnt) function->addChild(std::move(stmnt) );
		else {
			Global::errStack.push("Could not build valid statement", &*iterator);
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
	ext->token = id;
	while(!getIf(TokenType::ParensClose) ) {
		id = getIf(TokenType::Identifier);
		if(id) {
			Type type;
			type.name = id->value;
			type.isPtr = getIf(TokenType::Multiply);
			auto arg = getIf(TokenType::Identifier);
			ext->signature.paramNames.push_back(arg ? arg->value : "");
			ext->signature.parameters.push_back(type);
		} else if(getIf(TokenType::Variadic) ) {
			ext->signature.parameters.push_back({"...", false});
		} else {
			return unexpected();
		}
		if(!getIf(TokenType::Comma) ) {
			if(getIf(TokenType::ParensClose) ) {
				break;
			} else {
				return unexpected();
			}
		}
	}

	Type result;
	id = getIf(TokenType::Identifier);
	if(id) {
		result.name = id->value;
		result.isPtr = getIf(TokenType::Multiply);
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
	mayParseAssign = true;
	auto stmnt = std::make_unique<StatementAstNode>();
	Token *token = getIf(TokenType::Identifier);

	if(token) {	//Declaration?
		Type type {
			token->value,			//Type name
			getIf(TokenType::Multiply)	//If ptr
		};
		auto id = getIf(TokenType::Identifier);
		if(id) {
			auto decl = std::make_unique<VariableDeclareAstNode>();
			decl->type = type;
			decl->identifier = id->value;
			//TODO: Token could be either token or id
			decl->token = token;
			stmnt->addChild(std::move(decl) );

			//Declaration may include assignment
			if(getIf(TokenType::Assign) ) {
				unget();
				std::unique_ptr<ExpressionAstNode> idNode(new VariableAstNode(id->value) );
				auto binExpr = buildBinExpr(idNode);

				if(!binExpr) {
					return unexpected();
				}

				stmnt->addChild(std::move(binExpr) );
			}
			return stmnt;
		}

	}

	token = getIf(TokenType::Return);
	if(token) {
		auto ret = std::make_unique<ReturnAstNode>();
		if(getIf(TokenType::Terminator) ) {
			return ret;
		}

		auto expr = buildExpr();
		if(!expr) {
			return unexpected();
		}

		ret->addChild(std::move(expr) );
		return ret;
	}

	unget();
	auto expr = buildExpr();
	if(expr) {
		stmnt->addChild(std::move(expr) );
		return stmnt;
	}

	return unexpected();
}

std::unique_ptr<ExpressionAstNode> AstParser::buildCall(const std::string &identifier) {
	if(!getIf(TokenType::ParensOpen) ) {
		return nullptr;
	}
	auto call = std::make_unique<CallAstNode>(identifier);
	call->token = &*std::prev(iterator);
	auto expr = buildExpr();

	if(!expr) {
		if(!getIf(TokenType::ParensClose) ) {
			std::cerr << "Expected ')'\n";
			std::cerr << static_cast<size_t>(iterator->type) << " : " << iterator->value << '\n';
			return toExpr(unexpected() );
		}
		return call;
	}

	while(true) {
		call->addChild(std::move(expr) );
		if(!getIf(TokenType::Comma) ) {
			if(getIf(TokenType::ParensClose) ) {
				break;
			} else {
				return toExpr(unexpected() );
			}
		}
		expr = buildExpr();
	}

	return call;
}

std::unique_ptr<ExpressionAstNode> AstParser::buildExpr() {
	auto expr = buildPrimaryExpr();
	if(!expr) {
		return nullptr;
	}

	auto parent = buildBinExpr(expr);
	
	if(!parent) {
		return expr;
	}

	return parent;
}

std::unique_ptr<ExpressionAstNode> AstParser::buildPrimaryExpr() {
	std::unique_ptr<ExpressionAstNode> expr = nullptr;

	auto tok = getIf(TokenType::StringLiteral);
	if(tok) {
		mayParseAssign = false;
		expr = std::make_unique<StringAstNode>(tok->value);
	}

	if(!tok) {
		tok = getIf(TokenType::IntLiteral);
		if(tok) {
			mayParseAssign = false;
			expr = std::make_unique<IntAstNode>(tok->value);
		}
	}

	if(!tok) {
		tok = getIf(TokenType::Identifier);
		if(tok) {
			expr = buildCall(tok->value);
			if(!expr) {
				expr = std::make_unique<VariableAstNode>(tok->value);
			}
		}
	}


	if(!tok) {
		return nullptr;
	}

	expr->token = tok;
	return expr;
}

std::unique_ptr<ExpressionAstNode> AstParser::buildBinExpr(std::unique_ptr<ExpressionAstNode> &child) {
	std::unique_ptr<ExpressionAstNode> bin;

	//Check if bin
	if(getIf(TokenType::Assign) ) {
		if(!mayParseAssign) {
			Global::errStack.push("Constant expression or operator different from '=' "
					"may not appear to the left of an assignment", child->token);
			return nullptr;
		}
		bin = std::make_unique<BinExpressionAstNode>(TokenType::Assign);
	} else if(getIf(TokenType::Add) ) {
		bin = std::make_unique<BinExpressionAstNode>(TokenType::Add);
	} else if(getIf(TokenType::Multiply) ) {
		bin = std::make_unique<BinExpressionAstNode>(TokenType::Multiply);
	} else {
		//Not bin
		return nullptr;
	}

	//Is bin
	auto rhs = buildExpr();

	if(!rhs) {
		return toExpr(unexpected() );
	}

	//TODO: Precedence
	bin->addChild(std::move(child) );
	bin->addChild(std::move(rhs) );
	return bin;
}
