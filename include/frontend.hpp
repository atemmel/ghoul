#pragma once
#include "ast.hpp"
#include "symtable.hpp"

#include <string>

//TODO: Test this with loads of files, cyclic dependencies, etc
AstNode::Root performFrontendWork(const std::string &module, SymTable *symtable);
