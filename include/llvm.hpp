#pragma once
#include "ast.hpp"
#include "token.hpp"
#include "symtable.hpp"

#include "llvm/IR/Module.h"

#include <vector>
#include <set>
#include <string>
#include <memory>
#include <unordered_map>

struct ModuleInfo {
	std::string name;
	std::string fileName;
	std::string objName;
	std::set<std::string> links;
	std::unique_ptr<llvm::Module> module;
	std::unique_ptr<ToplevelAstNode> ast;
	SymTable *symtable;
};

struct Context {
	Context() : builder(context) {};
	llvm::LLVMContext context;
	llvm::IRBuilder<> builder;
};


class LLVMCodeGen : public AstVisitor {
public:
	void setModuleInfo(ModuleInfo *mi);
	void setContext(Context *ctx);
	void visit(ToplevelAstNode &node) override;
	void visit(LinkAstNode &node) override;
	void visit(StructAstNode &node) override;
	void visit(FunctionAstNode &node) override;
	void visit(ExternAstNode &node) override;
	void visit(VariableDeclareAstNode &node) override;
	void visit(ReturnAstNode &node) override;
	void visit(BranchAstNode &node) override;
	void visit(LoopAstNode &node) override;
	void visit(CallAstNode &node) override;
	void visit(BinExpressionAstNode &node) override;
	void visit(UnaryExpressionAstNode &node) override;
	void visit(CastExpressionAstNode &node) override;
	void visit(ArrayAstNode &node) override;
	void visit(IndexAstNode &node) override;
	void visit(MemberVariableAstNode &node) override;
	void visit(VariableAstNode &node) override;
	void visit(StringAstNode &node) override;
	void visit(IntAstNode &node) override;
	void visit(BoolAstNode &node) override;

private:
	template<typename T>
	using Map = std::unordered_map<std::string, T>;
	using Locals = Map<llvm::AllocaInst*>;

	llvm::Type *translateType(const Type &type);
	std::vector<FunctionAstNode*> getFuncsFromToplevel(ToplevelAstNode &node);
	void buildFunctionDefinitions(const std::vector<FunctionAstNode*> &funcs);
	void buildStructDefinitions(const std::vector<StructAstNode*> &structs);
	void clear();

	//Array related
	llvm::Value *allocateHeap(const Type &type, llvm::Value *length);
	llvm::Type *getArrayType(llvm::Type *type, const std::string &name);
	bool shouldAssignArray();
	void assignArray();
	llvm::Value *getArrayLength(llvm::Instruction *array);
	void setArrayLength(llvm::Instruction *array, llvm::Value *length);
	void popArray(llvm::Instruction *array);

	ModuleInfo *mi = nullptr;
	Context *ctx = nullptr;

	std::vector<llvm::Value*> callParams;
	std::vector<llvm::Value*> indicies;
	std::vector<llvm::Instruction*> instructions;

	Map<llvm::StructType*> structTypes;
	Map<llvm::Function*> functions;
	Map<llvm::Value*> values;
	Map<Locals> allLocals;
	Locals *locals = nullptr;
	llvm::Function *function = nullptr;

	llvm::Value *arrayLength = nullptr;
	const Type *lastType = nullptr;
	unsigned getAddrsVisited = 0;
	bool lastStatementVisitedWasReturn = false;
};

bool gen(ModuleInfo *mi, Context *ctx);

void tokensToBuilder(ModuleInfo *mi, Context *ctx);

void write(ModuleInfo *mi, Context *ctx);

void link(ModuleInfo *mi, Context *ctx);
