#include "llvm.hpp"
#include "global.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
//#include "llvm/ADT/VariadicFunction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>

#include <iostream>

void LLVMCodeGen::setModuleInfo(ModuleInfo *mi) {
	this->mi = mi;
}

void LLVMCodeGen::setContext(Context *ctx) {
	this->ctx = ctx;
}

void LLVMCodeGen::visit(ToplevelAstNode &node) {
	FunctionAstNode *main = nullptr;
	prepareToplevelNode(node);

	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void LLVMCodeGen::visit(StructAstNode &node) {

	std::vector<llvm::Type*> types;
	Type *struc = mi->symtable->hasStruct(node.name);

	if(!node.isVolatile) {
		auto &members = struc->members;
		std::sort(members.begin(), members.end(), [](Member &lhs, Member &rhs) {
			return lhs.type.size() > rhs.type.size();
		});
	}

	types.reserve(struc->members.size() );
	for(auto it = struc->members.begin(); it != struc->members.end(); it++) {
		types.push_back(translateType(it->type) );
	}

	structTypes[node.name]->setBody(types);
}

void LLVMCodeGen::visit(LinkAstNode &node) {
	mi->links.insert(node.string->value);
}

void LLVMCodeGen::visit(FunctionAstNode &node) {
	mi->symtable->setActiveFunction(node.signature.name);
	llvm::Function *func = function = functions[node.signature.name];
	llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx->context, "entrypoint", func);
	ctx->builder.SetInsertPoint(entry);

	//Uuum, okay?
	//TODO: Extend check to see if function is recursive
	if(node.signature.name == "main") {
		ctx->builder.CreateAlloca(llvm::Type::getInt32Ty(ctx->context) );
	}

	locals = &allLocals[node.signature.name];
	auto it = ctx->builder.GetInsertBlock();

	//TODO: Is this also needed?
	/*
	if(node.signature.name == "main") {
		auto alloca = new llvm::AllocaInst(llvm::Type::getInt32Ty(ctx->context), 0, "", it);
		ctx->builder.CreateStore(llvm::ConstantInt::get(ctx->context, llvm::APInt(32, 0, true) ), alloca );
	}
	*/

	for(auto &arg : func->args() ) {

		auto alloca = locals->insert(std::make_pair(arg.getName(), 
			new llvm::AllocaInst(arg.getType(), 0, arg.getName(), it) ) ).first;
		ctx->builder.CreateStore(&arg, alloca->second);
	}

	//Reset bool
	lastStatementVisitedWasReturn = false;
	for(const auto &child : node.children) {
		if(child) {
			clear();
			child->accept(*this);
		}
	}
	
	//Check to prevent double return/no return at all
	if(!lastStatementVisitedWasReturn) {
		ctx->builder.CreateRetVoid();
	}
}

void LLVMCodeGen::visit(ExternAstNode &node) {
	if(node.visited) {
		return;
	}

	node.visited = true;
	std::vector<llvm::Type*> callArgs;
	auto result = translateType(node.signature.returnType);
	for(const auto &type : node.signature.parameters) {
		callArgs.push_back(translateType(type) );
	}

	bool isVariadic = callArgs.empty() ? false : callArgs.back() == nullptr;
	if(isVariadic) {
		callArgs.pop_back();
	}

	llvm::ArrayRef<llvm::Type*> argsRef(callArgs);
	llvm::FunctionType *funcType = llvm::FunctionType::get(result, argsRef, isVariadic);
	mi->module->getOrInsertFunction(node.name, funcType);
}

void LLVMCodeGen::visit(VariableDeclareAstNode &node) {
	auto it = ctx->builder.GetInsertBlock();
	auto type = translateType(node.type);
	locals->insert(std::make_pair(node.identifier, 
				new llvm::AllocaInst(type, 0, node.identifier, it) ) );
	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void LLVMCodeGen::visit(ReturnAstNode &node) {
	callParams.clear();
	for(const auto &child : node.children) {
		child->accept(*this);
	}
	if(callParams.empty() ) {
		ctx->builder.CreateRetVoid();
	} else {
		ctx->builder.CreateRet(callParams.back() );
	} 
	lastStatementVisitedWasReturn = true;
}

void LLVMCodeGen::visit(BranchAstNode &node) {
	node.expr->accept(*this);
	llvm::BasicBlock *origin = ctx->builder.GetInsertBlock();
	llvm::BasicBlock *branch = llvm::BasicBlock::Create(ctx->context, "", function);
	llvm::BasicBlock *end = llvm::BasicBlock::Create(ctx->context, "", function);
	ctx->builder.CreateCondBr(callParams.back(), branch, end);

	ctx->builder.SetInsertPoint(branch);
	for(const auto &child : node.children) {
		child->accept(*this);
	}
	ctx->builder.CreateBr(end);
	ctx->builder.SetInsertPoint(end);
}

void LLVMCodeGen::visit(LoopAstNode &node) {
	llvm::BasicBlock *origin = ctx->builder.GetInsertBlock();

	if(node.loopPrefix) {
		node.loopPrefix->accept(*this);
		clear();
	}

	llvm::BasicBlock *cond = llvm::BasicBlock::Create(ctx->context, "", function);
	ctx->builder.CreateBr(cond);
	ctx->builder.SetInsertPoint(cond);

	node.expr->accept(*this);

	llvm::BasicBlock *branch = llvm::BasicBlock::Create(ctx->context, "", function);
	llvm::BasicBlock *end = llvm::BasicBlock::Create(ctx->context, "", function);

	ctx->builder.CreateCondBr(callParams.back(), branch, end);

	ctx->builder.SetInsertPoint(branch);
	for(const auto &child : node.children) {
		child->accept(*this);
	}

	if(node.loopSuffix) {
		node.loopSuffix->accept(*this);
		clear();
	}

	ctx->builder.CreateBr(cond);
	ctx->builder.SetInsertPoint(end);
}

void LLVMCodeGen::visit(CallAstNode &node) {
	auto oldParams = std::move(callParams);
	auto oldInsts = std::move(instructions);
	std::vector<llvm::Type*> callArgs;
	auto sig = mi->symtable->hasFunc(node.identifier);
	callArgs.reserve(sig->parameters.size() );
	for(auto &p : sig->parameters) {
		callArgs.push_back(translateType(p) );
	}

	bool isVariadic = sig->parameters.empty() ? false : sig->parameters.back().name == "...";

	llvm::ArrayRef<llvm::Type*> argsRef(callArgs);

	llvm::FunctionType *callType = llvm::FunctionType::get(translateType(sig->returnType), argsRef, isVariadic);
	auto func = mi->module->getFunction(node.identifier);
	
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}

	oldParams.push_back(ctx->builder.CreateCall(func, callParams) );
	callParams = std::move(oldParams);
	instructions = std::move(oldInsts);
}

void LLVMCodeGen::visit(BinExpressionAstNode &node) {
	auto insts = std::move(instructions);
	auto params = std::move(callParams);

	/*
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
	*/

	node.children.front()->accept(*this);
	const Type *lhsType = lastType;
	auto lhsLLVMType = translateType(*lhsType);

	node.children.back()->accept(*this);
	const Type *rhsType = lastType;
	auto rhsLLVMType = translateType(*rhsType);

	auto &lhs = callParams.front();
	auto &rhs = callParams.back();

	//TODO: Fix array/matrix assignment
	if(node.type == TokenType::Assign) {
		if(shouldAssignArray() ) {
			if(lhsIsRAArray) {
				assignRAArray();
			} else {
				assignArray();
			}
		} else if(lhsType->isStruct() && rhsType->isStruct() ) {
			auto inst = instructions.front();
			assignStruct(inst, rhs, lhsLLVMType);
		} else {
			auto inst = instructions.front();
			ctx->builder.CreateStore(callParams.back(), inst);
		}
		params.push_back(callParams.back() );
	} else if(node.type == TokenType::Add) {
		params.push_back(ctx->builder.CreateAdd(lhs, rhs) );
	} else if(node.type == TokenType::Multiply) {
		params.push_back(ctx->builder.CreateMul(lhs, rhs) );
	} else if(node.type == TokenType::Subtract) {
		params.push_back(ctx->builder.CreateSub(lhs, rhs) );
	} else if(node.type == TokenType::Divide) {
		params.push_back(ctx->builder.CreateSDiv(lhs, rhs) );
	} else if(node.type == TokenType::Equivalence) {
		params.push_back(ctx->builder.CreateICmpEQ(lhs, rhs) );
	} else if(node.type == TokenType::NotEquivalence) {
		params.push_back(ctx->builder.CreateICmpNE(lhs, rhs) );
	} else if(node.type == TokenType::Less) {
		params.push_back(ctx->builder.CreateICmpSLT(lhs, rhs) );
	} else if(node.type == TokenType::LessEquals) {
		params.push_back(ctx->builder.CreateICmpSLE(lhs, rhs) );
	} else if(node.type == TokenType::Greater) {
		params.push_back(ctx->builder.CreateICmpSGT(lhs, rhs) );
	} else if(node.type == TokenType::GreaterEquals) {
		params.push_back(ctx->builder.CreateICmpSGE(lhs, rhs) );
	} else if(node.type == TokenType::Push) {
		pushArray(instructions.back(), rhs);
		params.push_back(rhs);
	}

	callParams = std::move(params);
	instructions = std::move(insts);
}

void LLVMCodeGen::visit(UnaryExpressionAstNode &node) {
	if(node.type == TokenType::Multiply) {
		getAddrsVisited++;
	} 

	for(const auto &c : node.children) {
		c->accept(*this);
	}

	if(node.type == TokenType::Multiply) {
		getAddrsVisited--;
	} else if(node.type == TokenType::And) {
		std::vector<llvm::Value*> values = { 
			llvm::ConstantInt::get(ctx->context, llvm::APInt(32, 0, true) )
		};

		llvm::Instruction *gep = llvm::GetElementPtrInst::CreateInBounds(callParams.back(), values);
		if(!instructions.empty() ) {
			instructions.pop_back();
		}

		instructions.push_back(gep);
		ctx->builder.Insert(gep);
		callParams.back() = ctx->builder.CreateLoad(gep);
	} else if(node.type == TokenType::Ternary) {
		if(lhsIsRAArray) {
			callParams.back() = getRAArrayLength(instructions.back() );
		} else {
			callParams.back() = getArrayLength(instructions.back() );
		}
		instructions.pop_back();
	} else if(node.type == TokenType::Pop) {
		popArray(instructions.back() );
	} else if(node.type == TokenType::Tilde) {
		if(lhsIsRAArray) {
			freeRAArray(instructions.back() );
		} else {
			freeArray(instructions.back() );
		}
	}
}

void LLVMCodeGen::visit(CastExpressionAstNode &node) {
	llvm::Type *type = translateType(node.type);
	for(const auto &c : node.children) {
		c->accept(*this);
	}

	/*
	llvm::Instruction *cast = llvm::CastInst::Create(llvm::Instruction::BitCast, 
			callParams.back(), type);
	instructions.push_back(cast);
	ctx->builder.Insert(cast);
	*/

	auto v = ctx->builder.CreateIntCast(callParams.back(), type, true);

	callParams.back() = v;
}

void LLVMCodeGen::visit(ArrayAstNode &node) {
	if(lhsIsRAArray) {
		createRAArray(node);
	} else {
		createArray(node);
	}
}

void LLVMCodeGen::visit(IndexAstNode &node) {
	if(lhsIsRAArray) {
		indexRAArray(node);
	} else {
		indexArray(node);
	}
}

void LLVMCodeGen::visit(MemberVariableAstNode &node) {
	if(visitedRAAIndex) {
		visitedRAAIndex = false;
		indexRAArrayMember(node);
		return;
	}

	if(lastType->isPtr > 0) {
		for(int i = 0; i < lastType->isPtr; i++) {
			llvm::Instruction *alloca = ctx->builder.CreateLoad(instructions.back() );
			instructions.back() = alloca;
		}
	} 

	indicies.clear();
	unsigned u = mi->symtable->getMemberOffset(*lastType, node.name);
		indicies.push_back(llvm::ConstantInt::get(ctx->context, llvm::APInt(32, 0, true) ) );
	indicies.push_back(llvm::ConstantInt::get(ctx->context, llvm::APInt(32, u, true) ) );

	llvm::Instruction *gep = llvm::GetElementPtrInst::CreateInBounds(instructions.back(), indicies);
	instructions.back() = gep;
	ctx->builder.Insert(gep);

	lastType = mi->symtable->typeHasMember(*lastType, node.name);
	if(node.children.empty() ) {
		callParams.push_back(ctx->builder.CreateLoad(gep) );
		return;
	}
	
	for(const auto &c : node.children) {
		c->accept(*this);
	}
}

void LLVMCodeGen::visit(VariableAstNode &node) {
	auto ld = (*locals)[node.name];
	instructions.push_back(ld);
	lastType = mi->symtable->getLocal(node.name);
	lhsIsRAArray = lastType->realignedArray;

	if(node.children.empty() ) {
		if(getAddrsVisited > 0 || lastType->isStruct() ) {
			callParams.push_back(ld);
		} else {
			callParams.push_back(ctx->builder.CreateLoad(ld) );
		}
	} else {
		node.children.front()->accept(*this);
	}
}

void LLVMCodeGen::visit(StringAstNode &node) {
	auto it = values.find(node.value);
	if(it == values.end() ) {
		auto value = ctx->builder.CreateGlobalStringPtr(node.value);
		values[node.value] = value;
		callParams.push_back(value);
	} else {
		callParams.push_back(it->second);
	}
}

void LLVMCodeGen::visit(IntAstNode &node) {
	auto type = llvm::IntegerType::getInt32Ty(ctx->context);
	callParams.push_back(static_cast<llvm::Value*>(llvm::ConstantInt::get(type, 
		llvm::APInt(32, node.value) ) ) );
}

void LLVMCodeGen::visit(BoolAstNode &node) {
	auto type = llvm::IntegerType::getInt1Ty(ctx->context);
	callParams.push_back(static_cast<llvm::Value*>(llvm::ConstantInt::get(type,
		llvm::APInt(1, node.value) ) ) );
}

llvm::Type *LLVMCodeGen::translateType(const Type &ghoulType) {
	std::string name = ghoulType.name;
	return translateType(ghoulType, name);
}

llvm::Type *LLVMCodeGen::translateType(const Type &ghoulType, std::string &name) {
	llvm::Type *type = nullptr;
	if(ghoulType.name == "char") {
		type = ctx->builder.getInt8Ty();
	} else if(ghoulType.name == "bool") {
		type = ctx->builder.getInt1Ty();
	} else if(ghoulType.name == "int") {
		type = ctx->builder.getInt32Ty();
	} else if(ghoulType.name == "void") {
		type = ctx->builder.getVoidTy();
	} else if(ghoulType.name == "float") {
		type = ctx->builder.getFloatTy();
	} else if(ghoulType.name == "...") {
		return nullptr;	
	} else if(!ghoulType.arrayOf) {
		auto it = structTypes.find(ghoulType.name);
		if(it == structTypes.end() ) {
			std::cerr << ghoulType.string() << '\n';
			mi->module->print(llvm::errs(), nullptr);
			for(auto &s : structTypes) {
				std::cerr << s.first << '\n';
			}
			std::cerr << "Could not convert type successfully, aborting...\n";
			std::abort();
			return nullptr;
		}
		type = it->second;
	}

	if(!ghoulType.name.empty() ) {
		name = ghoulType.name;
	}

	if(ghoulType.arrayOf) {
		int isPtr = ghoulType.arrayOf->isPtr;
		ghoulType.arrayOf->isPtr = 0;	//Get underlying type if ptr
		type = translateType(*ghoulType.arrayOf, name);
		ghoulType.arrayOf->isPtr = isPtr;
		if(ghoulType.realignedArray) {
			type = getRAArrayType(type, ghoulType);
		} else {
			type = getArrayType(type, ghoulType);
		}
	}

	for(int i = 0; i < ghoulType.isPtr; i++) {
		type = type->getPointerTo();
	}

	return type;
}

void LLVMCodeGen::prepareToplevelNode(ToplevelAstNode &node) {
	buildStructDefinitions(node.structs);
	for(auto toplevel : node.toplevels) {
		prepareToplevelNode(*toplevel);
	}
	for(auto ext : node.externs) {
		visit(*ext);
	}
	buildFunctionDefinitions(node.functions);
}

void LLVMCodeGen::buildFunctionDefinitions(const std::vector<FunctionAstNode*> &funcs) {
	for(auto f : funcs) {
		std::vector<llvm::Type*> types;
		for(auto p : f->signature.parameters) {
			types.push_back(translateType(p) );
		}
		llvm::Type *returnType = translateType(f->signature.returnType);
		llvm::FunctionType *funcType = types.empty() 
			? llvm::FunctionType::get(returnType, false)
			: llvm::FunctionType::get(returnType, types, false);
		llvm::Function *func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, 
				f->signature.name, mi->module.get() );

		for(size_t i = 0; i < f->signature.paramNames.size(); i++) {
			func->arg_begin()[i].setName(f->signature.paramNames[i]);
		}

		func->setCallingConv(llvm::CallingConv::C);

		functions.insert(std::make_pair(f->signature.name, func) );

		allLocals.insert(std::make_pair(
					f->signature.name,
					Locals() ) );
	}
}

void LLVMCodeGen::buildStructDefinitions(const std::vector<StructAstNode*> &structs) {
	for(auto ptr : structs) {
		llvm::StructType *newStruct = llvm::StructType::create(ctx->context, ptr->name);
		structTypes.insert({ptr->name, newStruct});
	}
}

//TODO: Remove all calls to this
llvm::Value *LLVMCodeGen::allocateHeap(Type type, llvm::Value *length) {
	static llvm::Type *result = ctx->builder.getInt8Ty()->getPointerTo();
	static llvm::Type *argsRef = ctx->builder.getInt32Ty();
	static llvm::FunctionType *funcType = llvm::FunctionType::get(result, {argsRef}, false);
	const static llvm::FunctionCallee func = mi->module->getOrInsertFunction("malloc", funcType);

	auto memLength = ctx->builder.CreateMul(length, llvm::ConstantInt::get(ctx->builder.getInt32Ty(),
		llvm::APInt(32, type.size() ) ) );
	auto heapAlloc = ctx->builder.CreateCall(func, {memLength});
	auto cast = ctx->builder.CreatePointerCast(heapAlloc, translateType(type)->getPointerTo() );

	return cast;
}

llvm::Value *LLVMCodeGen::allocateHeap(llvm::Type *type, llvm::Value *length) {
	static llvm::Type *result = ctx->builder.getInt8Ty()->getPointerTo();
	static llvm::Type *argsRef = ctx->builder.getInt32Ty();
	static llvm::FunctionType *funcType = llvm::FunctionType::get(result, {argsRef}, false);
	const static llvm::FunctionCallee func = mi->module->getOrInsertFunction("malloc", funcType);

	auto memLength = ctx->builder.CreateMul(length, llvm::ConstantInt::get(ctx->builder.getInt32Ty(),
		llvm::APInt(32, mi->module->getDataLayout().getTypeAllocSize(type) ) ) );
	auto heapAlloc = ctx->builder.CreateCall(func, {memLength});
	auto cast = ctx->builder.CreatePointerCast(heapAlloc, type->getPointerTo() );

	return cast;
}

llvm::Value *LLVMCodeGen::reallocateHeap(Type type, llvm::Value *addr, llvm::Value *length) {
	static llvm::Type *result = ctx->builder.getVoidTy()->getPointerTo();
	static llvm::Type *ptrArg = ctx->builder.getVoidTy()->getPointerTo();
	static llvm::Type *countArg = ctx->builder.getInt32Ty();
	static llvm::FunctionType *funcType = llvm::FunctionType::get(result, {ptrArg, countArg}, false);
	const static llvm::FunctionCallee func = mi->module->getOrInsertFunction("realloc", funcType);

	type.isPtr--;
	auto memLength = ctx->builder.CreateMul(length, llvm::ConstantInt::get(ctx->builder.getInt32Ty(),
		llvm::APInt(32, type.size() ) ) );
	type.isPtr++;
	auto heapAlloc = ctx->builder.CreateCall(func, {addr, memLength});
	auto cast = ctx->builder.CreatePointerCast(heapAlloc, translateType(type) );

	return cast;
}

llvm::Type *LLVMCodeGen::getArrayType(llvm::Type *type, const Type &ghoulType) {
	std::string name = ghoulType.string();
	auto it = structTypes.find(name);
	llvm::StructType *arrayType;
	if(it == structTypes.end() ) {
		arrayType = llvm::StructType::create(ctx->context, name);
		arrayType->setBody({type->getPointerTo(), ctx->builder.getInt32Ty(), ctx->builder.getInt32Ty()});
		structTypes.insert({name, arrayType});
	} else {
		arrayType = it->second;
	}

	return arrayType;
}

void LLVMCodeGen::createArray(ArrayAstNode &node) {
	llvm::Type *arrayType = translateType(node.type);
	if(!node.length) {	//Only declared array type, null it
		llvm::Type *underlyingType = arrayType->getStructElementType(0)->getPointerTo();	//Hardcoded
		callParams.push_back(llvm::ConstantPointerNull::get(
			llvm::cast<llvm::PointerType>(underlyingType) ) );
		arrayLength = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), 0);
		return;
	} 

	node.length->accept(*this);
	arrayLength = callParams.back();
	callParams.pop_back();

	llvm::Value *heapAlloc = allocateHeap(arrayType, arrayLength);
	callParams.push_back(heapAlloc);
}

void LLVMCodeGen::indexArray(IndexAstNode &node) {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Instruction *addrFromStruct = llvm::GetElementPtrInst::CreateInBounds(instructions.back(), 
			{llvmZero, llvmZero} );	//First dereferences, second specifies member
	ctx->builder.Insert(addrFromStruct);
	auto load = ctx->builder.CreateLoad(addrFromStruct);

	auto prevType = lastType;
	auto oldVals = std::move(callParams);
	auto oldInsts = std::move(instructions);
	node.index->accept(*this);
	lastType = prevType;

	llvm::Instruction *gep = llvm::GetElementPtrInst::CreateInBounds(load, 
			{callParams.back()} );
	ctx->builder.Insert(gep);

	callParams = std::move(oldVals);
	instructions = std::move(oldInsts);

	if(instructions.size() == 1) {	//Edge case
		instructions.back() = gep;
	} else {
		*(instructions.end() - 2) = gep;
	}

	lastType = lastType->arrayOf.get();

	if(node.children.empty() ) {
		if(lastType->isStruct() ) {
			callParams.push_back(gep);
		} else {
			callParams.push_back(ctx->builder.CreateLoad(gep) );
		}
		return;
	}

	for(auto &c : node.children) {
		c->accept(*this);
	}
}

bool LLVMCodeGen::shouldAssignArray() {
	return arrayLength != nullptr;
}

void LLVMCodeGen::assignArray() {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Value *llvmOne = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 1) );
	llvm::Value *llvmTwo = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 2) );

	auto inst = instructions.front();
	auto addr = llvm::GetElementPtrInst::CreateInBounds(inst, {llvmZero, llvmZero} );
	auto size = llvm::GetElementPtrInst::CreateInBounds(inst, {llvmZero, llvmOne} );
	auto capacity = llvm::GetElementPtrInst::CreateInBounds(inst, {llvmZero, llvmTwo} );

	ctx->builder.Insert(addr);
	ctx->builder.Insert(size);
	ctx->builder.Insert(capacity);

	ctx->builder.CreateStore(callParams.back(), addr);
	ctx->builder.CreateStore(arrayLength, size);
	ctx->builder.CreateStore(arrayLength, capacity);
}

void LLVMCodeGen::clear() {
	callParams.clear();
	indicies.clear();
	instructions.clear();
	arrayLength = nullptr;
	lhsIsRAArray = false;
}

llvm::Value *LLVMCodeGen::getArrayLength(llvm::Instruction *array) {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Value *llvmOne = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 1) );
	auto size = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, llvmOne} );
	ctx->builder.Insert(size);
	return ctx->builder.CreateLoad(size);
}

void LLVMCodeGen::setArrayLength(llvm::Instruction *array, llvm::Value *length) {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Value *llvmOne = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 1) );
	auto size = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, llvmOne} );
	ctx->builder.Insert(size);
	ctx->builder.CreateStore(length, size);
}

void LLVMCodeGen::popArray(llvm::Instruction *array) {
	auto llvmOne = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 1) );
	auto length = getArrayLength(array);
	auto newLength = ctx->builder.CreateSub(length, llvmOne);
	setArrayLength(array, newLength);
}

void LLVMCodeGen::pushArray(llvm::Instruction *array, llvm::Value *value) {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Value *llvmOne = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 1) );
	llvm::Value *llvmTwo = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 2) );

	auto addr = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, llvmZero} );
	auto size = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, llvmOne} );
	auto capacity = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, llvmTwo} );

	ctx->builder.Insert(addr);
	ctx->builder.Insert(size);
	ctx->builder.Insert(capacity);

	llvm::BasicBlock *origin = ctx->builder.GetInsertBlock();

	llvm::BasicBlock *nullCheckBr = llvm::BasicBlock::Create(ctx->context, "", function);
	llvm::BasicBlock *capCheckBr = llvm::BasicBlock::Create(ctx->context, "", function);
	llvm::BasicBlock *capCheckBrImpl = llvm::BasicBlock::Create(ctx->context, "", function);
	llvm::BasicBlock *reallocBr = llvm::BasicBlock::Create(ctx->context, "", function);
	llvm::BasicBlock *end = llvm::BasicBlock::Create(ctx->context, "", function);

	auto loadedAddr = ctx->builder.CreateLoad(addr);

	//Nullcheck
	auto nullCheck = ctx->builder.CreateICmpEQ(loadedAddr,
		llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(loadedAddr->getType() ) ) );
	ctx->builder.CreateCondBr(nullCheck, nullCheckBr, capCheckBr);	// if addr is null

	ctx->builder.SetInsertPoint(nullCheckBr);

	auto dummyType = *lastType->arrayOf;
	dummyType.isPtr++;
	llvm::Value *mallocCall = allocateHeap(dummyType, llvmOne);

	ctx->builder.CreateStore(mallocCall, addr);
	ctx->builder.CreateStore(llvmOne, size);
	ctx->builder.CreateStore(llvmOne, capacity);
	auto index = llvm::GetElementPtrInst::CreateInBounds(addr, {llvmZero} );	//Assign
	ctx->builder.Insert(index);
	auto loadedIndex = ctx->builder.CreateLoad(index);
	ctx->builder.CreateStore(value, loadedIndex);

	ctx->builder.CreateBr(end);

	//Capacitycheck
	ctx->builder.SetInsertPoint(capCheckBr);

	auto loadedCap = ctx->builder.CreateLoad(capacity);
	auto loadedSize = ctx->builder.CreateLoad(size);
	auto newSize = ctx->builder.CreateAdd(loadedSize, llvmOne);
	auto capCheck = ctx->builder.CreateICmpSLE(newSize, loadedCap);

	ctx->builder.CreateCondBr(capCheck, capCheckBrImpl, reallocBr);	// if new size <= capacity
	ctx->builder.SetInsertPoint(capCheckBrImpl);
	loadedSize = ctx->builder.CreateLoad(size);
	index = llvm::GetElementPtrInst::CreateInBounds(addr, {loadedSize} );	//Assign
	ctx->builder.Insert(index);
	loadedIndex = ctx->builder.CreateLoad(index);
	ctx->builder.CreateStore(value, loadedIndex);
	ctx->builder.CreateStore(newSize, size);

	ctx->builder.CreateBr(end);

	//Realloc
	ctx->builder.SetInsertPoint(reallocBr);

	loadedCap = ctx->builder.CreateLoad(capacity);
	auto newCap = ctx->builder.CreateShl(loadedCap, llvmOne);
	dummyType.isPtr--;
	auto typeSize = dummyType.size();
	dummyType.isPtr++;
	auto numBytes = ctx->builder.CreateMul(
		llvm::ConstantInt::get(ctx->builder.getInt32Ty(), typeSize), loadedCap);
	ctx->builder.CreateStore(newCap, capacity);
	auto newMem = reallocateHeap(dummyType, loadedAddr, newCap);
	ctx->builder.CreateStore(newMem, addr);
	loadedSize = ctx->builder.CreateLoad(size);								//Resize
	newSize = ctx->builder.CreateAdd(loadedSize, llvmOne);
	index = llvm::GetElementPtrInst::CreateInBounds(newMem, {loadedSize});	//Assign
	ctx->builder.Insert(index);
	ctx->builder.CreateStore(value, index);
	ctx->builder.CreateStore(newSize, size);

	ctx->builder.CreateBr(end);

	ctx->builder.SetInsertPoint(end);
}

void LLVMCodeGen::freeArray(llvm::Instruction *array) {
	static llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	static llvm::Type *result = ctx->builder.getVoidTy();
	static llvm::Type *argsRef = ctx->builder.getVoidTy()->getPointerTo();
	static llvm::FunctionType *funcType = llvm::FunctionType::get(result, {argsRef}, false);
	const static llvm::FunctionCallee func = mi->module->getOrInsertFunction("free", funcType);

	auto addr = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, llvmZero} );

	ctx->builder.Insert(addr);
	auto loadedAddr = ctx->builder.CreateLoad(addr);
	auto cast = ctx->builder.CreatePointerCast(loadedAddr, argsRef);
	ctx->builder.CreateCall(func, cast);
}

void LLVMCodeGen::memcpy(llvm::Instruction *src, llvm::Instruction *dest, llvm::Value *length) {
	//TODO: Draft for assignment function
	llvm::BasicBlock *cond = llvm::BasicBlock::Create(ctx->context, "", function);
	ctx->builder.CreateBr(cond);
	ctx->builder.SetInsertPoint(cond);

	llvm::BasicBlock *branch = llvm::BasicBlock::Create(ctx->context, "", function);
	llvm::BasicBlock *end = llvm::BasicBlock::Create(ctx->context, "", function);

	auto it = ctx->builder.CreateAlloca(ctx->builder.getInt32Ty() );
	auto cmp = ctx->builder.CreateICmpEQ(it, length);

	ctx->builder.CreateCondBr(cmp, branch, end);

	ctx->builder.SetInsertPoint(branch);

	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );

	auto rhs = ctx->builder.CreateGEP(dest, {it, llvmZero} );
	auto lhs = ctx->builder.CreateGEP(src, {it, llvmZero} );
	auto loadedRhs = ctx->builder.CreateLoad(rhs);
	ctx->builder.CreateStore(loadedRhs, lhs);

	ctx->builder.CreateBr(cond);
	ctx->builder.SetInsertPoint(end);
}

llvm::Type *LLVMCodeGen::getRAArrayType(llvm::Type *type, const Type &ghoulType) {
	std::string name = ghoulType.string();
	auto it = structTypes.find(name);
	llvm::StructType *arrayType;
	if(it == structTypes.end() ) {
		arrayType = llvm::StructType::create(ctx->context, name);

		std::vector<llvm::Type*> body = { ctx->builder.getInt32Ty(), ctx->builder.getInt32Ty() };
		//const Type *verboseType = mi->symtable->hasStruct(ghoulType.name);
		//std::cerr << ghoulType.name << " " << verboseType << '\n';
		//type->

		/*
		for(const auto &memb : verboseType->members) {
			Type ty = memb.type;
			ty.isPtr++;
			body.push_back(translateType(ty) );
		}
		*/
	
		llvm::StructType *structTy = llvm::cast<llvm::StructType>(type);

		for(auto ty : structTy->elements() ) {
			body.push_back(ty->getPointerTo() );
		}

		arrayType->setBody(body);
		structTypes.insert({name, arrayType});
	} else {
		arrayType = it->second;
	}

	return arrayType;
}

void LLVMCodeGen::createRAArray(ArrayAstNode &node) {
	node.type.realignedArray = true;
	llvm::Type *arrayType = translateType(node.type);
	lastLLVMType = arrayType;

	if(!node.length) {	//Only declared array type, null it
		for(int i = 2; i < arrayType->getStructNumElements(); i++) {
			llvm::Type *underlyingType = arrayType->getStructElementType(i)->getPointerTo();	//Hardcoded
			callParams.push_back(llvm::ConstantPointerNull::get(
				llvm::cast<llvm::PointerType>(underlyingType) ) );
		}
		arrayLength = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), 0);
		return;
	} 

	//TODO: This
	node.length->accept(*this);
	arrayLength = callParams.back();
	callParams.pop_back();

	for(int i = 2; i < arrayType->getStructNumElements(); i++) {
		llvm::Type *underlyingType = arrayType->getStructElementType(i);
		auto ha = allocateHeap(underlyingType, arrayLength);
		callParams.push_back(ha);
	}
}

void LLVMCodeGen::indexRAArray(IndexAstNode &node) {
	auto prevType = lastType;
	auto oldVals = std::move(callParams);
	auto oldInsts = std::move(instructions);
	node.index->accept(*this);
	lastType = prevType;

	oldVals.push_back(callParams.back() );

	callParams = std::move(oldVals);
	instructions = std::move(oldInsts);

	lastType = lastType->arrayOf.get();
	visitedRAAIndex = true;
	for(auto &c : node.children) {
		c->accept(*this);
	}
}

void LLVMCodeGen::indexRAArrayMember(MemberVariableAstNode &node) {
	unsigned u = mi->symtable->getMemberOffset(*lastType, node.name);
	auto llvmZero = llvm::ConstantInt::get(ctx->context, llvm::APInt(32, 0, true) );
	auto structIndex = llvm::ConstantInt::get(ctx->context, llvm::APInt(32, u + 2, true) );
	auto arrayIndex = callParams.back();

	callParams.pop_back();

	llvm::Instruction *gep = llvm::GetElementPtrInst::CreateInBounds(instructions.back(), 
		{llvmZero, structIndex});

	ctx->builder.Insert(gep);
	auto loadedGep = ctx->builder.CreateLoad(gep);

	llvm::Instruction *element = llvm::GetElementPtrInst::CreateInBounds(loadedGep,
		{arrayIndex} );

	ctx->builder.Insert(element);

	instructions.back() = element;
	
	lastType = mi->symtable->typeHasMember(*lastType, node.name);

	if(node.children.empty() ) {
		callParams.push_back(ctx->builder.CreateLoad(element) );
		return;
	}
	
	for(const auto &c : node.children) {
		c->accept(*this);
	}
}

void LLVMCodeGen::assignRAArray() {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Value *llvmOne = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 1) );
	llvm::Value *llvmTwo = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 2) );

	auto inst = instructions.front();
	auto size = llvm::GetElementPtrInst::CreateInBounds(inst, {llvmZero, llvmZero} );
	auto capacity = llvm::GetElementPtrInst::CreateInBounds(inst, {llvmZero, llvmOne} );

	ctx->builder.Insert(size);
	ctx->builder.Insert(capacity);

	//ctx->builder.CreateStore(callParams.back(), addr);
	ctx->builder.CreateStore(arrayLength, size);
	ctx->builder.CreateStore(arrayLength, capacity);

	for(int i = lastLLVMType->getStructNumElements() - 1; i > 1; i--) {
		auto gep = llvm::GetElementPtrInst::CreateInBounds(inst, {llvmZero,
				llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, i) )});
		ctx->builder.Insert(gep);
		ctx->builder.CreateStore(callParams.back(), gep);
		callParams.pop_back();
	}
}

llvm::Value *LLVMCodeGen::getRAArrayLength(llvm::Instruction *raArray) {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	auto size = llvm::GetElementPtrInst::CreateInBounds(raArray, {llvmZero, llvmZero} );
	ctx->builder.Insert(size);
	return ctx->builder.CreateLoad(size);
}

void LLVMCodeGen::freeRAArray(llvm::Instruction *array) {
	static llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	static llvm::Type *result = ctx->builder.getVoidTy();
	static llvm::Type *argsRef = ctx->builder.getVoidTy()->getPointerTo();
	static llvm::FunctionType *funcType = llvm::FunctionType::get(result, {argsRef}, false);
	const static llvm::FunctionCallee func = mi->module->getOrInsertFunction("free", funcType);

	for(int i = 2; i < lastLLVMType->getStructNumElements(); i++) {
		auto index = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, i) );
		auto addr = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, index} );

		ctx->builder.Insert(addr);
		auto loadedAddr = ctx->builder.CreateLoad(addr);
		auto cast = ctx->builder.CreatePointerCast(loadedAddr, argsRef);
		ctx->builder.CreateCall(func, cast);
	}
}

void LLVMCodeGen::assignStruct(llvm::Instruction *lhs, llvm::Value *rhs, llvm::Type *type) {
	const static auto llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::StructType *strTy = llvm::cast<llvm::StructType>(type);

	for(int i = 0; i < strTy->getNumElements(); i++) {
		const auto gepIndex = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, i) );
		auto memberTy = strTy->getStructElementType(i);

		auto lhsGep = llvm::GetElementPtrInst::CreateInBounds(lhs, { llvmZero, gepIndex } );
		auto rhsGep = llvm::GetElementPtrInst::CreateInBounds(rhs, { llvmZero, gepIndex } );

		ctx->builder.Insert(lhsGep, "jens");
		ctx->builder.Insert(rhsGep, "jonte");

		auto rhsLoad = ctx->builder.CreateLoad(rhsGep);

		if(auto subStruct = llvm::dyn_cast<llvm::StructType>(memberTy) ) {
			assignStruct(lhsGep, rhsLoad, subStruct);
		} else {
			ctx->builder.CreateStore(rhsLoad, lhsGep);
		}
	}
}

bool gen(ModuleInfo *mi, Context *ctx) {
	std::cout << "Generating...\n";

	mi->module = std::make_unique<llvm::Module>(mi->name, ctx->context);

	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	LLVMCodeGen codeGen;
	codeGen.setContext(ctx);
	codeGen.setModuleInfo(mi);
	if(!mi->ast) {
		std::cerr << "Code generation not successful, aborting...\n";
		return false;
	}
	codeGen.visit(*mi->ast);

	if(Global::config.verbose || Global::config.verboseIR) {
		mi->module->print(llvm::errs(), nullptr);
	}

	write(mi, ctx);
	return true;
}

void write(ModuleInfo *mi, Context *ctx) {

	auto targetTriple = llvm::sys::getDefaultTargetTriple();
	mi->module->setTargetTriple(targetTriple);

	std::string err;
	auto target = llvm::TargetRegistry::lookupTarget(targetTriple, err);

	if(!target) {
		std::cerr << err << '\n';
	}

	auto cpu = "generic";
	auto features = "";

	llvm::TargetOptions opt;
	auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::Model::DynamicNoPIC);
	auto theTargetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, RM);

	mi->module->setDataLayout(theTargetMachine->createDataLayout() );
	mi->module->setTargetTriple(targetTriple);

	std::error_code ec;
	llvm::raw_fd_ostream dest(mi->objName, ec, llvm::sys::fs::F_None);

	llvm::legacy::PassManager pass;
	auto fileType = llvm::CGFT_ObjectFile;

	if(theTargetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType) ) {
		std::cerr << "TargetMachine cannot emit a file of this type\n";
		return;
	}

	pass.run(*mi->module);

	/*
	if(Global::config.verbose || Global::config.verboseIR) {
		std::cerr << "Optimized IR:\n";
		mi->module->print(llvm::errs(), nullptr);
	}
	*/

	dest.flush();

	std::cout << "Object written to " << mi->objName << '\n';
	link(mi, ctx);
}

void link(ModuleInfo *mi, Context *ctx) {
	std::cout << "Linking to " << mi->name << '\n';
	std::string link;
	for(const auto &str : mi->links) {
		link += " -l";
		link += str;
	}
	system((std::string("gcc -O0 ") + mi->objName + " -o " + mi->name + link).c_str() );
}
