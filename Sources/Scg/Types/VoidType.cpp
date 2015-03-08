/**
* @file Scg/Types/VoidType.cpp
*
* @copyright Copyright (C) 2014 Rafid Khalid Abdullah
*
* @license This file is released under Alusus Public License, Version 1.0.
* For details on usage and copying conditions read the full license in the
* accompanying license file or at <http://alusus.net/alusus_license_1_0>.
*/
//==============================================================================

#include <prerequisites.h>

// LLVM header files
#include <llvm/IR/Constants.h>

// Scg header files
#include <Types/VoidType.h>
#include <LlvmContainer.h>

namespace Scg
{
VoidType *VoidType::s_singleton = nullptr;

VoidType::VoidType() : typeSpec("void")
{
  this->llvmType = llvm::Type::getVoidTy(LlvmContainer::GetContext());
  if (s_singleton == nullptr)
    s_singleton = this;
}

void VoidType::InitCastingTargets() const
{
}

VoidType *VoidType::Get()
{
  // PERFORMANCE: What is the impact of running an unnecessary if statement
  // thousands of times?
  if (s_singleton == nullptr) {
    s_singleton = new VoidType();
  }
  return s_singleton;
}
}
