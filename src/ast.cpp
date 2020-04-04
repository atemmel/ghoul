#include "ast.hpp"

#include "astprint.hpp"
#include "frontend.hpp"
#include "global.hpp"
#include "llvm.hpp"
#include "symtable.hpp"
#include "utils.hpp"

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

void LinkAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

StructAstNode::StructAstNode(const std::string &name) 
	: name(name) {
}

void StructAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
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

void VariableDeclareAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void ReturnAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void BranchAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void LoopAstNode::accept(AstVisitor &visitor) {
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

UnaryExpressionAstNode::UnaryExpressionAstNode(Token *token) 
	: type(token->type) {
	precedence = Token::precedence(type);
	this->token = token;
}

void UnaryExpressionAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

CastExpressionAstNode::CastExpressionAstNode(const Type &type) 
 	: type(type) {
}

void CastExpressionAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void ArrayAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

void IndexAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

MemberVariableAstNode::MemberVariableAstNode(const std::string &name)
	: name(name) {
	precedence = Token::precedence(TokenType::Identifier);
}

void MemberVariableAstNode::accept(AstVisitor &visitor) {
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

IntAstNode::IntAstNode(int value) 
	: value(value) {
	precedence = Token::precedence(TokenType::IntLiteral);
}

IntAstNode::IntAstNode(const std::string &value) {
	isIntLiteral(value, this->value);
	precedence = Token::precedence(TokenType::IntLiteral);
}

void IntAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

BoolAstNode::BoolAstNode(bool value) 
	: value(value) {
}

void BoolAstNode::accept(AstVisitor &visitor) {
	visitor.visit(*this);
}

AstNode::Root AstParser::buildTree(Tokens &&tokens, SymTable *symtable) {
	this->tokens = tokens;
	this->symtable = symtable;
	iterator = this->tokens.begin();
	return buildTree();
}


#ifndef NDEBUG

AstNode::Child AstParser::panic(const char *file, int line) {
	isPanic = true;
	if(iterator == tokens.cend() ) {
		Global::errStack.push("Unexpected end of file", &*std::prev(iterator) );
	} else {
		Global::errStack.push(std::string("Unexpected token: '") + iterator->value + '\'', &*iterator);
	}
	Global::errStack.push(std::string(file) + " at " + std::to_string(line), nullptr);
	discardUntil(TokenType::Terminator);	//Remember, no dupes!
	discardWhile(TokenType::Terminator);
	return nullptr;
}

#else

AstNode::Child AstParser::panic() {
	isPanic = true;
	if(iterator == tokens.cend() ) {
		Global::errStack.push("Unexpected end of file", &*std::prev(iterator) );
	} else {
		Global::errStack.push(std::string("Unexpected token: '") + iterator->value + '\'', &*iterator);
	}
	discardUntil(TokenType::Terminator);	//Remember, no dupes!
	discardWhile(TokenType::Terminator);
	return nullptr;
}
#endif

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

void AstParser::discardUntil(TokenType type) {
	while(iterator != tokens.end() && iterator->type != type) {
		iterator++;
	}
}

AstNode::Root AstParser::mergeTrees(AstNode::Root &&lhs, AstNode::Root &&rhs) {
	auto product = std::make_unique<ToplevelAstNode>();

	product->children = std::move(lhs->children);
	product->functions = std::move(lhs->functions);
	product->externs = std::move(lhs->externs);

	//We can be smart about this
	product->children.reserve(product->children.size() + rhs->children.size() );
	product->functions.reserve(product->functions.size() + rhs->functions.size() );
	product->externs.reserve(product->externs.size() + rhs->externs.size() );

	//These need to be moved
	std::move(rhs->children.begin(), rhs->children.end(), std::back_inserter(product->children) );

	//These can be copied
	std::copy(rhs->functions.begin(), rhs->functions.end(), std::back_inserter(product->functions) );
	std::copy(rhs->externs.begin(), rhs->externs.end(), std::back_inserter(product->externs) );

	return product;
}

AstNode::Root AstParser::buildTree() {
	auto toplevel = std::make_unique<ToplevelAstNode>();

	root = toplevel.get();

	for(;;) {
		discardWhile(TokenType::Terminator);
		auto link = buildLink();
		if(link) {
			toplevel->addChild(std::move(link) );
			continue;
		}
		auto import = buildImport();
		if(import) {
			//toplevel = mergeTrees(std::move(toplevel), std::move(import) );
			toplevel->addChild(std::move(import) );
			continue;
		}
		else if(getIf(TokenType::Function) ) {
			auto func = buildFunction();
			if(!func) {
				return nullptr;
			}
			//TODO: If func is empty, a parsing error has occured
			//Log this somehow for error messages
			auto fptr = static_cast<FunctionAstNode*>(func.get() );
			toplevel->addFunction(fptr);
			toplevel->addChild(std::move(func) );
		} else if(getIf(TokenType::Extern) ) {
			auto ext = buildExtern();
			if(!ext) {
				return nullptr;
			}
			auto eptr = static_cast<ExternAstNode*>(ext.get() );
			toplevel->addExtern(eptr);
			toplevel->addChild(std::move(ext) );
		} else if(getIf(TokenType::Struct) ) {
			auto struc = buildStruct();
			if(!struc) {
				return nullptr;
			}
			auto struptr = static_cast<StructAstNode*>(struc.get() );
			toplevel->structs.push_back(struptr);
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

AstNode::Root AstParser::buildImport() {
	Token *token = getIf(TokenType::Import);
	if(!token) {
		return nullptr;
	}

	Token *file = getIf(TokenType::StringLiteral);
	if(!file) {
		return toRoot(unexpected() );
	}

	AstNode::Root sub = performFrontendWork(file->value, symtable);
	return sub;
}

AstNode::Child AstParser::buildLink() {
	Token *token = getIf(TokenType::Link);
	if(!token) {
		return nullptr;
	}

	auto link = std::make_unique<LinkAstNode>();
	link->token = token;

	token = getIf(TokenType::StringLiteral);
	if(!token) {
		return unexpected();
	}

	auto str = std::make_unique<StringAstNode>(token->value);
	str->token = token;

	link->string = str.get();
	link->addChild(std::move(str) );

	return link;
}

AstNode::Child AstParser::buildStruct() {
	Token *token = getIf(TokenType::Identifier);
	if(!token || !getIf(TokenType::BlockOpen) ) {
		return unexpected();
	}

	auto struc = std::make_unique<StructAstNode>(token->value);
	struc->token = token;

	discardWhile(TokenType::Terminator);

	AstNode::Child child;
	for(;;) {
		Token *id = getIf(TokenType::Identifier);
		if(!id) {
			break;
		}
		unget();
		child = buildDecl();
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
		/*
		auto typeId = getIf(TokenType::Identifier);
		if(!typeId) {
			return unexpected();
		}

		type = buildType(typeId);
		*/
		if(!buildType(type) ) {
			return unexpected();
		}
		
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

	/*
	auto ret = getIf(TokenType::Identifier);
	if(ret) {
		function->signature.returnType = buildType(ret);
	} else {	//Implicit void
		function->signature.returnType = {"void"};
	}
	*/

	Type type;
	if(!buildType(type) ) {
		type.name = "void";
	} 

	function->signature.returnType = type;
	
	if(!getIf(TokenType::BlockOpen) ) {
		return unexpected();
	}

	discardWhile(TokenType::Terminator);
	auto stmnt = buildStatement();
	while(stmnt) {
		function->addChild(std::move(stmnt) );
		discardWhile(TokenType::Terminator);
		stmnt = buildStatement();
	}

	if(!getIf(TokenType::BlockClose) ) {
		return isPanic ? nullptr : unexpected();
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
		/*
		id = getIf(TokenType::Identifier);
		if(id) {
			Type type = buildType(id);
			auto arg = getIf(TokenType::Identifier);
			ext->signature.paramNames.push_back(arg ? arg->value : "");
			ext->signature.parameters.push_back(type);
		} else if(getIf(TokenType::Variadic) ) {
			ext->signature.parameters.push_back({"...", false});
			ext->signature.paramNames.push_back("");
		*/
		Type type;
		if(buildType(type) ) {
			auto arg = getIf(TokenType::Identifier);
			ext->signature.paramNames.push_back(arg ? arg->value : "");
			ext->signature.parameters.push_back(type);
		} else if(getIf(TokenType::Variadic) ) {
			type.name = "...";
			type.arrayOf.reset();
			ext->signature.parameters.push_back(type);
			ext->signature.paramNames.push_back("");
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
	/*
	id = getIf(TokenType::Identifier);
	if(id) {
		result = buildType(id);
	} else {
		result.name = "void";
	}
	*/

	if(iterator->type == TokenType::Terminator) {
		result.name = "void";
	} else if(!buildType(result) ) {
		return unexpected();
	} 

	ext->signature.returnType = result;

	if(!getIf(TokenType::Terminator) ) {
		return unexpected();
	}

	return ext;
}

AstNode::Child AstParser::buildStatement() {
	mayParseAssign = true;
	auto decl = buildDecl();
	if(decl) {
		return decl;
	}
	Token *token = getIf(TokenType::Identifier);

	if(token) {	//Declaration?
		unget();
	}

	token = getIf(TokenType::Return);
	if(token) {
		auto ret = std::make_unique<ReturnAstNode>();
		if(getIf(TokenType::Terminator) ) {
			return ret;
		}

		auto expr = buildExpr();
		if(!expr) {
			Global::errStack.push("Could not build valid statement", &*iterator);
			return unexpected();
		}

		ret->addChild(std::move(expr) );
		return ret;
	}

	auto node = buildBranch();
	if(node) {
		return node;
	}
	node = buildLoop();
	if(node) {
		return node;
	}

	auto expr = buildExpr();
	if(expr) {
		if(expr->precedence != 0) {
			if(Global::errStack.empty() ) {
				Global::errStack.push("Stray expression", expr->token);
			}
			return nullptr;
		} else {
			return expr;
		}
	} 

	return nullptr;
}

//AstNode::Child AstParser::buildDecl(Token *token) {
AstNode::Child AstParser::buildDecl() {
	/*
	Token *token = getIf(TokenType::Identifier);
	Type type;
	if(token) {
		type = buildType(token);	
	} else if(!getIf(TokenType::Var) ){
		return nullptr;
	}
	*/

	Type type;
	Token *token = &*iterator;
	bool builtType = buildType(type);
	if(!builtType && !getIf(TokenType::Var) ) { 
		return nullptr;
	}

	auto id = getIf(TokenType::Identifier);
	if(!id) {
		unget();
		return nullptr;
	}

	auto decl = std::make_unique<VariableDeclareAstNode>();
	decl->identifier = id->value;
	//TODO: Token could be either token or id
	decl->token = token;

	//Declaration may include assignment
	if(getIf(TokenType::Assign) ) {
		unget();
		AstNode::Expr idNode(new VariableAstNode(id->value) );
		auto assign = buildAssignExpr(idNode);

		if(!assign) {
			return unexpected();
		}

		decl->addChild(std::move(assign) );
	} else if(!builtType) {
		Global::errStack.push("'var' declaration expects an assignment", &*iterator);
	}

	decl->type = type;
	return decl;
}

AstNode::Child AstParser::buildBranch() {
	Token *tok = getIf(TokenType::If);
	if(!tok) {
		return nullptr;
	}

	AstNode::Expr expr = buildExpr();

	if(!expr) {
		return unexpected();
	}

	auto br = std::make_unique<BranchAstNode>();
	br->expr = std::move(expr);

	tok = getIf(TokenType::BlockOpen);
	if(!tok) {
		tok = getIf(TokenType::Then);
		if(tok) {
			auto stmnt = buildStatement();
			if(stmnt) {
				br->addChild(std::move(stmnt) );
				return br;
			}
		}
		return unexpected();
	}

	tok = getIf(TokenType::Terminator);
	if(!tok) {
		return unexpected();
	}

	discardWhile(TokenType::Terminator);
	auto stmnt = buildStatement();
	while(stmnt) {
		br->addChild(std::move(stmnt) );
		discardWhile(TokenType::Terminator);
		stmnt = buildStatement();
	}

	if(!getIf(TokenType::BlockClose) ) {
		return unexpected();
	}

	return br;
}

AstNode::Child AstParser::buildLoop() {
	auto loop = buildWhile();
	if(!loop) {
		loop = buildFor();
	}

	if(!loop) {
		return nullptr;
	}

	Token *tok = getIf(TokenType::BlockOpen);
	if(!tok) {
		return unexpected();
	}

	tok = getIf(TokenType::Terminator);
	if(!tok) {
		return unexpected();
	}

	discardWhile(TokenType::Terminator);
	auto stmnt = buildStatement();
	while(stmnt) {
		loop->addChild(std::move(stmnt) );
		discardWhile(TokenType::Terminator);
		stmnt = buildStatement();
	}

	if(!getIf(TokenType::BlockClose) ) {
		return unexpected();
	}		

	return loop;
}

AstNode::Child AstParser::buildWhile() {
	Token *tok = getIf(TokenType::While);
	if(!tok) {
		return nullptr;
	}

	AstNode::Expr expr = buildExpr();

	if(!expr) {
		return unexpected();
	}

	auto loop = std::make_unique<LoopAstNode>();
	loop->expr = std::move(expr);
	loop->token = tok;
	return loop;
}

AstNode::Child AstParser::buildFor() {
	Token *tok = getIf(TokenType::For);
	if(!tok) {
		return nullptr;
	}

	auto loop = std::make_unique<LoopAstNode>();
	loop->token = tok;

	mayParseAssign = true;
	auto decl = buildDecl();
	if(decl) {
		loop->loopPrefix = std::move(decl);
	}

	tok = getIf(TokenType::Semicolon);
	if(!tok) {
		return unexpected();
	}

	AstNode::Expr expr = buildExpr();

	if(!expr) {
		return unexpected();
	}

	loop->expr = std::move(expr);

	tok = getIf(TokenType::Semicolon);
	if(!tok) {
		return unexpected();
	}

	mayParseAssign = true;
	expr = buildExpr();
	if(expr) {
		loop->loopSuffix = std::move(expr);
	}

	return loop;
}

AstNode::Expr AstParser::buildCall(const std::string &identifier) {
	if(!getIf(TokenType::ParensOpen) ) {
		return nullptr;
	}
	auto call = std::make_unique<CallAstNode>(identifier);
	call->token = &*std::prev(iterator);
	auto expr = buildExpr();

	if(!expr) {
		if(!getIf(TokenType::ParensClose) ) {
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
				//return panic ? nullptr : toExpr(unexpected() );
				return toExpr(unexpected() );
			}
		}
		expr = buildExpr();
	}

	return call;
}

AstNode::Expr AstParser::buildExpr() {
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

AstNode::Expr AstParser::buildPrimaryExpr() {
	AstNode::Expr expr = nullptr;

	expr = buildPrefixUnaryExpr();
	if(expr) {
		return expr;
	}

	expr = buildArray();
	if(expr) {
		return expr;
	}

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
		bool val = false;
		tok = getIf(TokenType::False);
		if(!tok) {
			val = true;
			tok = getIf(TokenType::True);
		}

		if(tok) {
			mayParseAssign = false;
			expr = std::make_unique<BoolAstNode>(val);
		}
	}

	if(!tok) {
		tok = getIf(TokenType::Identifier);
		if(tok) {
			expr = buildCall(tok->value);
			if(!expr) {
				expr = buildVariableExpr(tok);
			}
		}
	}

	if(!tok) {
		return nullptr;
	}

	expr->token = tok;
	auto parent = buildPostfixUnaryExpr(expr);
	if(parent) {
		return parent;
	}
	return expr;
}

AstNode::Expr AstParser::buildVariableExpr(Token *token) {
	auto var = std::make_unique<VariableAstNode>(token->value);


	auto child = buildMemberExpr();
	if(child) {
		var->addChild(std::move(child) );
	} else {
		//TODO: Build index
		auto index = buildIndex();
		if(index) {
			var->addChild(std::move(index) );
			mayParseAssign = true;
		}
	}

	return var;
}

AstNode::Expr AstParser::buildMemberExpr() {
	if(!getIf(TokenType::Member) ) {
		return nullptr;
	}

	Token *id = getIf(TokenType::Identifier);
	if(!id) {
		return toExpr(unexpected() );
	}

	auto member = std::make_unique<MemberVariableAstNode>(id->value);

	//TODO: Build index

	auto child = buildMemberExpr();

	if(child) {
		member->addChild(std::move(child) );
	}

	return member;
}

AstNode::Expr AstParser::buildAssignExpr(AstNode::Expr &lhs) {
	AstNode::Expr bin;
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

AstNode::Expr AstParser::buildBinOp() {
	Token *token = getIf(TokenType::Add);

	//TODO: Refactor this...?
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
		token = getIf(TokenType::Equivalence);
	}
	if(!token) {
		token = getIf(TokenType::NotEquivalence);
	}
	if(!token) {
		token = getIf(TokenType::Greater);
	}
	if(!token) {
		token = getIf(TokenType::GreaterEquals);
	}
	if(!token) {
		token = getIf(TokenType::Less);
	}
	if(!token) {
		token = getIf(TokenType::LessEquals);
	} 
	if(!token) {
		token = getIf(TokenType::Push);
	}

	if(!token) {
		return nullptr;
	}

	auto bin = std::make_unique<BinExpressionAstNode>(token->type);
	bin->token = token;
	return bin;
}

AstNode::Expr AstParser::buildBinExpr(AstNode::Expr &child) {

	AstNode::Expr bin = buildBinOp();
	if(!bin) {
		return nullptr;
	}

	auto val = buildPrimaryExpr();
	if(!val) {
		return toExpr(unexpected() );
	}

	//https://en.wikipedia.org/wiki/Shunting-yard_algorithm

	std::vector<AstNode::Expr> valStack;
	std::vector<AstNode::Expr> opStack;

	valStack.push_back(std::move(child) );
	valStack.push_back(std::move(val) );
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

AstNode::Expr AstParser::buildPrefixUnaryOp() {
	Token *tok = nullptr;
	if(iterator == tokens.cend() ) {
		return nullptr;
	}
	switch(iterator->type) {
		case TokenType::Multiply:
		case TokenType::And:
			tok = &*iterator;
			break;
		default:
			return nullptr;
	}

	iterator++;
	return std::make_unique<UnaryExpressionAstNode>(tok);
}

AstNode::Expr AstParser::buildPrefixUnaryExpr() {
	auto un = buildPrefixUnaryOp();
	if(!un) {
		un = buildCast();
	}

	if(!un) {
		return nullptr;
	}

	auto expr = buildPrimaryExpr();

	if(!expr) {
		return toExpr(unexpected() );
	}

	un->addChild(std::move(expr) );

	return un;
}

AstNode::Expr AstParser::buildPostfixUnaryOp() {
	Token *tok = nullptr;
	if(iterator == tokens.cend() ) {
		return nullptr;
	}
	switch(iterator->type) {
		case TokenType::Ternary:
		case TokenType::Pop:
			tok = &*iterator;
			break;
		default:
			return nullptr;
	}

	iterator++;
	return std::make_unique<UnaryExpressionAstNode>(tok);
}

AstNode::Expr AstParser::buildPostfixUnaryExpr(AstNode::Expr &child) {
	auto un = buildPostfixUnaryOp();
	if(!un) {
		return nullptr;
	}

	un->addChild(std::move(child) );
	return un;
}

AstNode::Expr AstParser::buildCast() {
	if(!getIf(TokenType::Less) ) {
		return nullptr;
	}

	/*
	Token *token = getIf(TokenType::Identifier);
	if(!token) {
		return toExpr(unexpected() );
	}

	auto cast = std::make_unique<CastExpressionAstNode>(buildType(token) );
	*/

	Type type;
	if(!buildType(type) ) {
		return toExpr(unexpected() );
	}

	auto cast = std::make_unique<CastExpressionAstNode>(type);

	if(!getIf(TokenType::Greater) ) {
		return nullptr;
	}

	return cast;
}

AstNode::Expr AstParser::buildArray() {
	if(!getIf(TokenType::ArrayStart) ) {
		return nullptr;
	}

	auto array = std::make_unique<ArrayAstNode>();
	array->length = buildExpr();

	if(!getIf(TokenType::ArrayEnd) ) {
		return toExpr(unexpected() );
	}

	/*
	Token *token = getIf(TokenType::Identifier);
	if(!token) {
		return toExpr(unexpected() );
	}

	array->type = buildType(token);
	array->type.isArray = true;
	*/

	Type underlyingType; 

	if(!buildType(underlyingType) ) {
		return toExpr(unexpected() );
	}

	array->type.arrayOf = std::make_unique<Type>(underlyingType);

	return array;
}

AstNode::Expr AstParser::buildIndex() {
	Token *start = getIf(TokenType::ArrayStart);
	if(!start) {
		return nullptr;
	}

	auto node = std::make_unique<IndexAstNode>();
	node->token = start;
	auto index = buildExpr();

	if(!index) {
		return toExpr(unexpected() );
	}

	if(!getIf(TokenType::ArrayEnd) ) {
		return toExpr(unexpected() );
	}

	node->index = std::move(index);
	return node;
}

bool AstParser::buildType(Type &type) {
	auto checkpoint = iterator;

	if(getIf(TokenType::ArrayStart) ) {
		if(!getIf(TokenType::ArrayEnd) ) { 
			iterator = checkpoint;
			return false;
		}

		if(getIf(TokenType::Multiply) ) {
			type.isPtr++;
		}

		Type subType;

		if(!buildType(subType) ) {
			iterator = checkpoint;
			return false;
		}

		type.arrayOf = std::make_unique<Type>(subType);
	} 

	if(type.arrayOf) {
		return true;
	}
	
	auto id = getIf(TokenType::Identifier);
	if(id) {
		type.name = id->value;
		while(getIf(TokenType::Multiply) ) {
			type.isPtr++;
		}
		return true;
	} 
	
	iterator = checkpoint;
	return false;
}

/*
Type AstParser::buildType(Token *token) {
	Type type;
	type.name = token->value;
	while(getIf(TokenType::Multiply) ) {
		type.isPtr++;
	}
	return type;
}
*/
