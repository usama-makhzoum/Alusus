/**
 * @file Scg/Instructions/DefineFunction.h
 *
 * @copyright Copyright (C) 2014 Rafid Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <http://alusus.net/alusus_license_1_0>.
 */
//==============================================================================

#ifndef __DefineFunction_h__
#define __DefineFunction_h__

// Scg header files
#include <Functions/UserDefinedFunction.h>
#include <Instructions/Instruction.h>
#include <Types/ValueType.h>
#include <Types/ValueTypeSpec.h>

#include <llvm_fwd.h>

namespace Scg
{
  class Block;
  class ValueTypeSpec;
}

namespace Scg
{
/**
 * Represents a function definition, i.e. a prototype and body.
 */
class DefineFunction : public Instruction
{
  //! A string containing the name of the function.
  std::string name;
  //! The return value type of the function.
  ValueTypeSpec *returnType;
  //! An array containing the types and names of the function's arguments.
  VariableDefinitionArray arguments;
  //! A pointer to the block containing the body of the function.
  Block *body;

public:
  /**
   * Construct a function with the given name, arguments, and body.
   * @param[in] name        The name of the function.
   * @param[in] returnType  The return value type of the function.
   * @param[in] arguments   The arguments of the function.
   * @param[in] body        The body of the function.
   */
  DefineFunction(Core::Basic::Char const *name, ValueTypeSpec *returnType,
    const VariableDefinitionArray &arguments, Block *body);

  //! Class destructor.
  ~DefineFunction();

  /**
   * Retrieves the name of the function to be defined by this instruction.
   * @return The name of the function to be defined.
   */
  const std::string &GetName() const { return name; }

  /**
   * Retrieves the return type of the function.
   * @return The return type of the function.
   */
  const ValueTypeSpec *GetReturnType() const { return returnType; }
  ValueTypeSpec *GetReturnType() { return returnType; }

  /**
   * Retrieves the arguments of the function to be defined by this instruction.
   * This is an array type-name tuple, one array item for each argument.
   * @return The arguments of the function.
   */
  const VariableDefinitionArray &GetArguments() const { return arguments; }

  /**
   * Retrieves a pointer to the Alusus function defined by this instruction.
   *
   * @return A pointer to the Alusus function defined by this instruction.
   */
  const UserDefinedFunction *GetDefinedFunction() const { return (UserDefinedFunction *)(children[0]); }
  UserDefinedFunction *GetDefinedFunction() { return (UserDefinedFunction *)(children[0]); }

  //! @copydoc Expression::PreGenerateCode()
  virtual CodeGenerationStage PreGenerateCode();

  //! @copydoc Expression::PostGenerateCode()
  virtual CodeGenerationStage PostGenerateCode();

  //! @copydoc Expression::ToString()
  virtual std::string ToString();
};
}

#endif // __DefineFunction_h__
