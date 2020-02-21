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

StructAstNode::StructAstNode(const std::string &name) 
	: name(name) {
}

void StructAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
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
			if(!func) return nullptr;
			//TODO: If func is empty, a parsing error has occured
			//Log this somehow for error messages
			auto fptr = static_cast<FunctionAstNode*>(func.get() );
			toplevel->addFunction(fptr);
			toplevel->addChild(std::move(func) );
		} else if(getIf(TokenType::Extern) ) {
			auto ext = buildExtern();
			if(!ext) return nullptr;
			auto eptr = static_cast<ExternAstNode*>(ext.get() );
			toplevel->addExtern(eptr);
			toplevel->addChild(std::move(ext) );
		} else if(getIf(TokenType::Struct) ) {
			auto struc = buildStruct();
			if(!struc) return nullptr;
			toplevel->addChild(std::move(struc) );
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

AstNode::Child AstParser::buildStruct() {
	Token *token = getIf(TokenType::Identifier);
	if(!token || !getIf(TokenType::BlockOpen) ) {
		return unexpected();
	}

	auto struc = std::make_unique<StructAstNode>(token->value);

	discardWhile(TokenType::Terminator);

	AstNode::Child child;
	for(;;) {
		Token *id = getIf(TokenType::Identifier);
		if(!id) {
			break;
		}
		child = buildDecl(id);
		if(!child) {
			return unexpected();
		}
		struc->addChild(std::move(child) );
		discardWhile(TokenType::Terminator);
	}

	discardWhile(TokenType::Terminator);
	if(!getIf(TokenType::BlockClose) ) {
		return unexpected();
	}

	return struc;
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
		auto decl = buildDecl(token);
		if(decl) {
			stmnt->addChild(std::move(decl) );
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
		if(expr->precedence != 0) {
			Global::errStack.push("Stray expression", expr->token);
			return nullptr;
		} else {
			stmnt->addChild(std::move(expr) );
			return stmnt;
		}
	}

	return unexpected();
}

AstNode::Child AstParser::buildDecl(Token *token) {
	Type type {
		token->value,			//Type name
		getIf(TokenType::Multiply)	//If ptr
	};
	auto id = getIf(TokenType::Identifier);
	if(!id) {
		return nullptr;
	}

	auto decl = std::make_unique<VariableDeclareAstNode>();
	decl->type = type;
	decl->identifier = id->value;
	//TODO: Token could be either token or id
	decl->token = token;

	//Declaration may include assignment
	if(getIf(TokenType::Assign) ) {
		unget();
		std::unique_ptr<ExpressionAstNode> idNode(new VariableAstNode(id->value) );
		auto assign = buildAssignExpr(idNode);

		if(!assign) {
			return unexpected();
		}

		decl->addChild(std::move(assign) );
	}

	return decl;
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

	auto parent = buildAssignExpr(expr);
	if(!parent) {
		mayParseAssign = false;
		parent = buildBinExpr(expr);
	}
	
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

std::unique_ptr<ExpressionAstNode> AstParser::buildAssignExpr(std::unique_ptr<ExpressionAstNode> &lhs) {
	std::unique_ptr<ExpressionAstNode> bin;
	Token *token = getIf(TokenType::Assign);
	if(!token) {
		return nullptr;
	}

	if(!mayParseAssign) {
		Global::errStack.push("Constant expression or operator different from '=' "
		"may not appear to the left of an assignment", lhs->token);
		return nullptr;
	}

	bin = std::make_unique<BinExpressionAstNode>(TokenType::Assign);
	bin->token = token;

	auto rhs = buildExpr();

	if(!rhs) {
		return toExpr(unexpected() );
	}

	bin->addChild(std::move(lhs) );
	bin->addChild(std::move(rhs) );

	auto parent = buildAssignExpr(bin);

	if(parent) {
		return parent;
	}

	return bin;
}

std::unique_ptr<ExpressionAstNode> AstParser::buildBinOp() {
	Token *token = getIf(TokenType::Add);
	if(!token) {
		token = getIf(TokenType::Multiply);
	}
	if(!token) {
		token = getIf(TokenType::Subtract);
	}
	if(!token) {
		token = getIf(TokenType::Divide);
	}

	if(!token) {
		return nullptr;
	}

	auto bin = std::make_unique<BinExpressionAstNode>(token->type);
	bin->token = token;
	return bin;
}

std::unique_ptr<ExpressionAstNode> AstParser::buildBinExpr(std::unique_ptr<ExpressionAstNode> &child) {

	std::unique_ptr<ExpressionAstNode> bin = buildBinOp();
	if(!bin) {
		return nullptr;
	}

	auto val = buildPrimaryExpr();
	if(!val) {
		return toExpr(unexpected() );
	}

	//https://en.wikipedia.org/wiki/Shunting-yard_algorithm

	std::vector<std::unique_ptr<ExpressionAstNode> > valStack, opStack;

	valStack.push_back(std::move(val) );
	valStack.push_back(std::move(child) );
	opStack.push_back(std::move(bin) );

	bin = buildBinOp();

	while(bin) {
		if(bin->precedence <= opStack.back()->precedence) {
			auto rhs = std::move(valStack.back() );
			valStack.pop_back();
			auto lhs = std::move(valStack.back() );
			valStack.pop_back();
			auto op = std::move(opStack.back() );
			opStack.pop_back();

			op->addChild(std::move(lhs) );
			op->addChild(std::move(rhs) );

			valStack.push_back(std::move(op) );

		}
		opStack.push_back(std::move(bin) );

		val = buildPrimaryExpr();

		if(!val) {
			return toExpr(unexpected() );
		}

		valStack.push_back(std::move(val) );

		bin = buildBinOp();
	}

	while(!opStack.empty() ) {
		auto rhs = std::move(valStack.back() );
		valStack.pop_back();
		auto lhs = std::move(valStack.back() );
		valStack.pop_back();
		auto op = std::move(opStack.back() );
		opStack.pop_back();

		op->addChild(std::move(lhs) );
		op->addChild(std::move(rhs) );

		valStack.push_back(std::move(op) );
	}

	auto result = std::move(valStack.back() );
	return result;
}
