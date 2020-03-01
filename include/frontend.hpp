#pragma once
#include "ast.hpp"
#include "symtable.hpp"

#include <string>

AstNode::Root performFrontendWork(const std::string &filename, SymTable *symtable);
