#include "llvm.hpp"
#include "global.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
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

#include <string>
#include <vector>

void LLVMCodeGen::setModuleInfo(ModuleInfo *mi) {
	this->mi = mi;
}

void LLVMCodeGen::setContext(Context *ctx) {
	this->ctx = ctx;
}

void LLVMCodeGen::visit(ToplevelAstNode &node) {
	FunctionAstNode *main = nullptr;
	buildFunctionDefinitions(node.functions);

	for(const auto &child : node.children) {
		child->accept(*this);
	}
}

void LLVMCodeGen::visit(FunctionAstNode &node) {

	llvm::Function *func = mi->functions[node.name];
	llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx->context, "entrypoint", func);
	ctx->builder.SetInsertPoint(entry);

	//TODO: Add args to locals

	locals = &allLocals[node.name];

	//Content goes here
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
	
	//TODO: Change return type & value to match node.signature
	ctx->builder.CreateRetVoid();
}

void LLVMCodeGen::visit(ExternAstNode &node) {
	std::vector<llvm::Type*> callArgs;
	auto result = translateType(node.signature.returnType);
	for(const auto &type : node.signature.parameters) {
		callArgs.push_back(translateType(type) );
	}

	llvm::ArrayRef<llvm::Type*> argsRef(callArgs);
	llvm::FunctionType *funcType = llvm::FunctionType::get(result, argsRef, false);
	mi->module->getOrInsertFunction(node.name, funcType);
}

void LLVMCodeGen::visit(StatementAstNode &node) {
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
}

void LLVMCodeGen::visit(VariableDeclareAstNode &node) {
	auto it = ctx->builder.GetInsertBlock();
	auto type = translateType(node.type);
	locals->insert(std::make_pair(node.identifier, 
		new llvm::AllocaInst(type, 0, node.identifier, it) ) );
}

void LLVMCodeGen::visit(CallAstNode &node) {
	callParams.clear();
	visitedVariables.clear();
	std::vector<llvm::Type*> callArgs;
	auto sig = mi->symtable.hasFunc(node.identifier);
	for(auto &p : sig->parameters) {
		callArgs.push_back(translateType(p) );
	}

	llvm::ArrayRef<llvm::Type*> argsRef(callArgs);

	llvm::FunctionType *callType = llvm::FunctionType::get(translateType(sig->returnType), argsRef, false);
	auto putsFunc = mi->module->getOrInsertFunction(node.identifier, callType);
	
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
	llvm::ArrayRef<llvm::Value*> paramRef(callParams);
	ctx->builder.CreateCall(putsFunc, paramRef);
}

void LLVMCodeGen::visit(BinExpressionAstNode &node) {
	auto vars = std::move(visitedVariables);
	auto params = std::move(callParams);
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}

	if(node.type == TokenType::Assign) {
		VariableAstNode *node = visitedVariables.front();
		ctx->builder.CreateStore(callParams.back(), (*locals)[node->name]);
		params.push_back(callParams.back() );
	} else if(node.type == TokenType::Add) {
		params.push_back(ctx->builder.CreateAdd(callParams.front(), callParams.back() ) );
	} else if(node.type == TokenType::Multiply) {
		params.push_back(ctx->builder.CreateMul(callParams.front(), callParams.back() ) );
	}
	callParams = std::move(params);
	visitedVariables = std::move(vars);
}

void LLVMCodeGen::visit(VariableAstNode &node) {
	visitedVariables.push_back(&node);
	callParams.push_back(ctx->builder.CreateLoad((*locals)[node.name]) );
}

void LLVMCodeGen::visit(StringAstNode &node) {
	auto it = mi->values.find(node.value);
	if(it == mi->values.end() ) {
		auto value = ctx->builder.CreateGlobalStringPtr(node.value);
		mi->values[node.value] = value;
		callParams.push_back(value);
	} else {
		callParams.push_back(it->second);
	}
}

void LLVMCodeGen::visit(IntAstNode &node) {
	auto type = llvm::IntegerType::getInt32Ty(ctx->context);
	callParams.push_back(static_cast<llvm::Value*>(llvm::ConstantInt::get(type, 
		llvm::APInt(32, std::to_string(node.value), 10) ) ) );
}

llvm::Type *LLVMCodeGen::translateType(const Type &astType) const {
	llvm::Type *type = nullptr;
	if(astType.name == "char") {
		type = ctx->builder.getInt8Ty();
	} else if(astType.name == "int") {
		type = ctx->builder.getInt32Ty();
	} else if(astType.name == "void") {
		type = ctx->builder.getVoidTy();
	} else {
		return nullptr;
	}

	return astType.isPtr ? type->getPointerTo() : type;
}

std::vector<FunctionAstNode*> LLVMCodeGen::getFuncsFromToplevel(ToplevelAstNode &node) {
	std::vector<FunctionAstNode*> funcs;
	for(auto it = node.children.begin(); it != node.children.end(); it++) {
		auto ptr = dynamic_cast<FunctionAstNode*>(it->get() );
		if(ptr) {
			funcs.push_back(ptr);
		}
	}
	return funcs;
}

void LLVMCodeGen::buildFunctionDefinitions(const std::vector<FunctionAstNode*> &funcs) {
	for(auto f : funcs) {
		llvm::FunctionType *funcType = llvm::FunctionType::get(ctx->builder.getVoidTy(), false);
		llvm::Function *func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, 
				f->name, mi->module.get() );
		mi->functions.insert(std::make_pair(f->name, func) );

		allLocals.insert(std::make_pair(
					f->name,
					Locals() ) );
	}
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

	if(Global::config.verbose) mi->module->print(llvm::errs(), nullptr);
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
	auto RM = llvm::Optional<llvm::Reloc::Model>();
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
	dest.flush();

	std::cout << "Object written to " << mi->objName << '\n';
	link(mi, ctx);
}

void link(ModuleInfo *mi, Context *ctx) {
	std::cout << "Linking to " << mi->name << '\n';
	system((std::string("gcc -O0 -static ") + mi->objName + " -o " + mi->name).c_str() );
}
