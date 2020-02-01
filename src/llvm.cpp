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
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
}

void LLVMCodeGen::visit(FunctionAstNode &node) {
	llvm::FunctionType *funcType = llvm::FunctionType::get(ctx->builder.getVoidTy(), false);
	llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, 
			node.identifier, mi->module.get() );

	llvm::BasicBlock *entry = llvm::BasicBlock::Create(ctx->context, "entrypoint", mainFunc);
	ctx->builder.SetInsertPoint(entry);

	//Content goes here
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
	
	ctx->builder.CreateRetVoid();
}

void LLVMCodeGen::visit(StatementAstNode &node) {
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
}

void LLVMCodeGen::visit(CallAstNode &node) {
	callParams.clear();
	std::vector<llvm::Type*> callArgs;
	callArgs.push_back(ctx->builder.getInt8Ty()->getPointerTo());

	llvm::ArrayRef<llvm::Type*> argsRef(callArgs);

	llvm::FunctionType *putsType = llvm::FunctionType::get(ctx->builder.getInt32Ty(), argsRef, false);
	auto putsFunc = mi->module->getOrInsertFunction(node.identifier, putsType);
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
	llvm::ArrayRef<llvm::Value*> paramRef(callParams);
	ctx->builder.CreateCall(putsFunc, paramRef);
}

void LLVMCodeGen::visit(ExpressionAstNode &node) {
	for(const auto &child : node.children) {
		if(child) {
			child->accept(*this);
		}
	}
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
