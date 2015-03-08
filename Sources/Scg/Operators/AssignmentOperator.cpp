/**
 * @file Scg/Operators/AssignmentOperator.cpp
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
#include <llvm/IR/IRBuilder.h>

// Scg files
#include <Containers/Block.h>
#include <Operators/Content.h>
#include <Operators/AssignmentOperator.h>

using namespace llvm;

namespace Scg
{
const ValueTypeSpec *AssignmentOperator::GetValueTypeSpec() const
{
  return GetRHS()->GetValueTypeSpec();
}

Expression::CodeGenerationStage AssignmentOperator::GenerateCode()
{
  BLOCK_CHECK;

  auto irb = GetBlock()->GetIRBuilder();

  // TODO: Don't use dynamic_cast.
  auto lhs = dynamic_cast<Content*>(GetLHS());
  if (lhs == nullptr)
  throw EXCEPTION(InvalidOperationException, "The left-hand side of an "
      "assignment must be the content of a pointer.");
  if (lhs->GetCodeGenerationStage() == Expression::CodeGenerationStage::CodeGeneration)
  	if (lhs->CallGenerateCode() == Expression::CodeGenerationStage::CodeGeneration)
  		return Expression::CodeGenerationStage::CodeGeneration;

  // Find the value of the right-hand side expression.
  if (GetRHS()->GetCodeGenerationStage() == Expression::CodeGenerationStage::CodeGeneration)
  	if (GetRHS()->CallGenerateCode() == Expression::CodeGenerationStage::CodeGeneration)
  		return Expression::CodeGenerationStage::CodeGeneration;
  auto rhs = GetRHS()->GetGeneratedLlvmValue();
  if (rhs == 0)
    throw EXCEPTION(InvalidValueException,
        ("Right-hand side of '=' doesn't evaluate to a value: "
            + GetRHS()->ToString()).c_str());

  auto targetType = GetLHS()->GetValueTypeSpec()->ToValueType(*GetModule());
  auto sourceType = GetRHS()->GetValueTypeSpec()->ToValueType(*GetModule());
  if (!sourceType->IsImplicitlyCastableTo(targetType)) {
    throw EXCEPTION(CompilationErrorException,
        "The right-hand side cannot be assigned to the left-hand side because "
        "it has a different type.");
  }

  // Add a store instruction to set the value of the variable.
  if (sourceType != targetType) {
    this->llvmStoreInst = irb->CreateStore(
        sourceType->CreateCastInst(irb, rhs, targetType), lhs->GetLlvmPointer());
  } else {
    this->llvmStoreInst = irb->CreateStore(rhs, lhs->GetLlvmPointer());
  }

  // The generated value for an assignment is the right-hand side of the
  // assignment itself.
  generatedLlvmValue = rhs;

  return Expression::GenerateCode();
}

//------------------------------------------------------------------------------

Expression::CodeGenerationStage AssignmentOperator::PostGenerateCode()
{
  if (!this->llvmStoreInst->hasNUses(0)) {
    // Cannot delete the instruction yet; stay in PostCodeGeneration stage.
    return CodeGenerationStage::PostCodeGeneration;
  }
  this->llvmStoreInst->eraseFromParent();
  this->llvmStoreInst = nullptr;

  return Expression::PostGenerateCode();
}

//------------------------------------------------------------------------------

std::string AssignmentOperator::ToString()
{
  return GetLHS()->ToString() + " := " + GetRHS()->ToString();
}
}
