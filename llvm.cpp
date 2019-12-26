#include "llvm.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
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

#include <map>
#include <string>
#include <vector>

static llvm::LLVMContext context;
static llvm::IRBuilder<> bob(context);
static std::unique_ptr<llvm::Module> module;
//static std::map<std::string, llvm::Value*> values;

void gen() {
	std::cout << "Generating...\n";

	module = std::make_unique<llvm::Module>("biggie smalls", context);

	llvm::FunctionType *funcType = llvm::FunctionType::get(bob.getVoidTy(), false);
	llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module.get() );

	llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entrypoint", mainFunc);
	bob.SetInsertPoint(entry);

	llvm::Value *helloWorld = bob.CreateGlobalStringPtr("Hello world!\n");

	std::vector<llvm::Type*> putsArgs;
	putsArgs.push_back(bob.getInt8Ty()->getPointerTo());

	llvm::ArrayRef<llvm::Type*> argsRef(putsArgs);

	llvm::FunctionType *putsType = llvm::FunctionType::get(bob.getInt32Ty(), argsRef, false);
	auto putsFunc = module->getOrInsertFunction("puts", putsType);

	bob.CreateCall(putsFunc, helloWorld);
	bob.CreateRetVoid();

	module->print(llvm::errs(), nullptr);
	write();
}

void write() {
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	auto targetTriple = llvm::sys::getDefaultTargetTriple();
	module->setTargetTriple(targetTriple);

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

	module->setDataLayout(theTargetMachine->createDataLayout() );
	module->setTargetTriple(targetTriple);

	std::error_code ec;
	std::string filename = "output";
	std::string relocname = filename + ".o";
	llvm::raw_fd_ostream dest(relocname, ec, llvm::sys::fs::F_None);

	llvm::legacy::PassManager pass;
	auto fileType = llvm::TargetMachine::CGFT_ObjectFile;

	if(theTargetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType) ) {
		std::cerr << "TargetMachine cannot emit a file of this type\n";
		return;
	}

	pass.run(*module);
	dest.flush();

	std::cout << "Object written to " << filename << '\n';
	system((std::string("gcc -static ") + relocname + " -o " + filename).c_str() );
}
