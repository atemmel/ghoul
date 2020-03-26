#include "llvm.hpp"
#include "global.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/VariadicFunction.h"
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
	buildStructDefinitions(node.structs);
	buildFunctionDefinitions(node.functions);

	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void LLVMCodeGen::visit(StructAstNode &node) {
	std::vector<llvm::Type*> types;
	const Type *struc = mi->symtable->hasStruct(node.name);
	for(const auto &member : struc->members) {
		types.push_back(translateType(member.type) );
	}
	//TODO: Sort members for compact layout
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

	llvm::ArrayRef<llvm::Value*> paramRef(callParams);
	oldParams.push_back(ctx->builder.CreateCall(func, paramRef) );
	callParams = std::move(oldParams);
	instructions = std::move(oldInsts);
}

void LLVMCodeGen::visit(BinExpressionAstNode &node) {
	auto insts = std::move(instructions);
	auto params = std::move(callParams);
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}

	auto &lhs = callParams.front();
	auto &rhs = callParams.back();

	if(node.type == TokenType::Assign) {
		if(shouldAssignArray() ) {
			assignArray();
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
	} 

	callParams = std::move(params);
	instructions = std::move(insts);
}

void LLVMCodeGen::visit(UnaryExpressionAstNode &node) {
	if(node.type == TokenType::Multiply) {
		getAddrsVisited++;
	} else if(node.type == TokenType::Ternary) {
		std::cout << "Felt cute, might die later\n";
		getLengthsVisited++;
	}

	for(const auto &c : node.children) {
		c->accept(*this);
	}
	std::cout << "Did not die!";

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
		getLengthsVisited--;
		callParams.back() = getArrayLength(instructions.back() );
		instructions.pop_back();
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
	if(!node.length) {	//Only declared array type, null it
		llvm::Type *arrayType = translateType(node.type);
		llvm::Type *underlyingType = arrayType->getStructElementType(0);	//Hardcoded
		callParams.push_back(llvm::ConstantPointerNull::get(
			llvm::cast<llvm::PointerType>(underlyingType) ) );
		arrayLength = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), 0);
		return;
	} 

	node.length->accept(*this);
	arrayLength = callParams.back();
	callParams.pop_back();

	node.type.isArray = false;	//TODO: Correct this
	node.type.isPtr++;
	llvm::Value *heapAlloc = allocateHeap(node.type, arrayLength);
	callParams.push_back(heapAlloc);
}

void LLVMCodeGen::visit(IndexAstNode &node) {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Instruction *addrFromStruct = llvm::GetElementPtrInst::CreateInBounds(instructions.back(), 
			{llvmZero, llvmZero} );	//First dereferences, second specifies member
	ctx->builder.Insert(addrFromStruct);
	auto load = ctx->builder.CreateLoad(addrFromStruct);
	node.index->accept(*this);

	llvm::Instruction *gep = llvm::GetElementPtrInst::CreateInBounds(load, 
			{callParams.back()} );
	ctx->builder.Insert(gep);

	if(instructions.size() == 1) {	//Edge case
		instructions.back() = gep;
	} else {
		*(instructions.end() - 2) = gep;
	}

	callParams.back() = ctx->builder.CreateLoad(gep);
}

void LLVMCodeGen::visit(MemberVariableAstNode &node) {
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

	if(node.children.empty() ) {
		callParams.push_back(ctx->builder.CreateLoad(gep) );
		return;
	}
	
	lastType = mi->symtable->typeHasMember(*lastType, node.name);
	for(const auto &c : node.children) {
		c->accept(*this);
	}
}

void LLVMCodeGen::visit(VariableAstNode &node) {
	auto ld = (*locals)[node.name];
	instructions.push_back(ld);

	if(node.children.empty() ) {
		const Type *type = mi->symtable->getLocal(node.name);
		if(getAddrsVisited > 0) {
			callParams.push_back(ld);
		} else {
			callParams.push_back(ctx->builder.CreateLoad(ld) );
		}
	} else {
		lastType = mi->symtable->getLocal(node.name);
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
	} else {
		auto it = structTypes.find(ghoulType.name);
		if(it == structTypes.end() ) {
			std::cerr << ghoulType.string() << '\n';
			std::cerr << "UHOH\n";
			return nullptr;
		}
		type = it->second;
	}

	for(int i = 0; i < ghoulType.isPtr; i++) {
		type = type->getPointerTo();
	}

	if(ghoulType.isArray) {	//TODO: Nu-uh
		type = getArrayType(type, ghoulType.name);
	}

	return type;
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

llvm::Value *LLVMCodeGen::allocateHeap(const Type &type, llvm::Value *length) {
	static llvm::Type *result = ctx->builder.getInt8Ty()->getPointerTo();
	static llvm::Type *argsRef = ctx->builder.getInt32Ty();
	static llvm::FunctionType *funcType = llvm::FunctionType::get(result, {argsRef}, false);
	const static llvm::FunctionCallee func = mi->module->getOrInsertFunction("malloc", funcType);

	auto memLength = ctx->builder.CreateMul(length, llvm::ConstantInt::get(ctx->builder.getInt32Ty(),
		llvm::APInt(32, type.size() ) ) );
	auto heapAlloc = ctx->builder.CreateCall(func, {memLength});
	auto cast = ctx->builder.CreatePointerCast(heapAlloc, translateType(type) );

	return cast;
}

llvm::Type *LLVMCodeGen::getArrayType(llvm::Type *type, const std::string &name) {
	std::string newName = "[]" + name;
	auto it = structTypes.find(newName);
	llvm::StructType *arrayType;
	if(it == structTypes.end() ) {
		arrayType = llvm::StructType::create(ctx->context, newName);
		arrayType->setBody({type->getPointerTo(), ctx->builder.getInt32Ty(), ctx->builder.getInt32Ty()});
		structTypes.insert({newName, arrayType});
	} else {
		arrayType = it->second;
	}

	return arrayType;
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
}

llvm::Value *LLVMCodeGen::getArrayLength(llvm::Instruction *array) {
	llvm::Value *llvmZero = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 0) );
	llvm::Value *llvmOne = llvm::ConstantInt::get(ctx->builder.getInt32Ty(), llvm::APInt(32, 1) );
	auto size = llvm::GetElementPtrInst::CreateInBounds(array, {llvmZero, llvmOne} );
	ctx->builder.Insert(size);
	return ctx->builder.CreateLoad(size);
}

bool gen(ModuleInfo *mi, Context *ctx) {
	std::cout << "Generating...\n";

	mi->module = std::make_unique<llvm::Module>(mi->name, ctx->context);

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
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

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
	auto fileType = llvm::TargetMachine::CGFT_ObjectFile;

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
