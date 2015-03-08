/**
 * @file Tests/ScgTests/binary_operators_tests.cpp
 *
 * @copyright Copyright (C) 2014 Rafid Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <http://alusus.net/alusus_license_1_0>.
 */
//==============================================================================

// Alusus header files
#include <core.h>
#include <scg.h>
#include <simple_test.h>

using namespace Scg;

namespace Tests { namespace ScgTests
{

bool TestAssignmentOperator()
{
  LlvmContainer::Initialize();

  // Create a type specification for an integer.
  // Create the main function.
  auto mainBody = new Block({
      new DefineVariable(CreateTypeSpecByName("int"), "a"),
      new DefineVariable(CreateTypeSpecByName("int"), "b"),
      new AssignmentOperator(
          new Content(new PointerToVariable("a")),
          new Content(new PointerToVariable("b"))),
      new Return(new IntegerConst(0))
  });
  auto main = new DefineFunction("main", new ValueTypeSpecByName("int"),
      VariableDefinitionArray(), mainBody);

  // Create the module.
  auto module = new Module("TestAssignmentOperator");
  module->AppendExpression(main);
  auto program = new Program();
  program->AddModule(module);
  std::cout << program->Compile() << std::endl;
  std::cout << "TestAssignmentOperator succeeded." << std::endl;
  delete program;

  LlvmContainer::Finalize();

  return true;
}

bool TestBinaryOperators()
{
  LlvmContainer::Initialize();

  // Create a type specification for an integer.
  // Create the main function.
  auto mainBody = new Block({
      new DefineVariable(CreateTypeSpecByName("int"), "a"),
      new DefineVariable(CreateTypeSpecByName("int"), "b"),
      new CallFunction("__add", new List({
        new Content(new PointerToVariable("a")),
        new Content(new PointerToVariable("b"))
      })),
      new CallFunction("__sub", new List({
        new Content(new PointerToVariable("a")),
        new Content(new PointerToVariable("b"))
      })),
      new CallFunction("__mul", new List({
        new Content(new PointerToVariable("a")),
        new Content(new PointerToVariable("b"))
      })),
      new CallFunction("__div", new List({
        new Content(new PointerToVariable("a")),
        new Content(new PointerToVariable("b"))
      })),
      new BinaryOperator(BinaryOperator::GREATERTHAN,
          new Content(new PointerToVariable("a")),
          new Content(new PointerToVariable("b"))),
      new BinaryOperator(BinaryOperator::GREATERTHAN_EQUAL,
          new Content(new PointerToVariable("a")),
          new Content(new PointerToVariable("b"))),
      new BinaryOperator(BinaryOperator::LESSTHAN,
          new Content(new PointerToVariable("a")),
          new Content(new PointerToVariable("b"))),
      new BinaryOperator(BinaryOperator::LESSTHAN_EQUAL,
          new Content(new PointerToVariable("a")),
          new Content(new PointerToVariable("b"))),
      new BinaryOperator(BinaryOperator::EQUAL,
          new Content(new PointerToVariable("a")),
          new Content(new PointerToVariable("b"))),
      new BinaryOperator(BinaryOperator::NOTEQUAL,
          new Content(new PointerToVariable("a")),
          new Content(new PointerToVariable("b"))),
      new Return(new IntegerConst(0))
  });
  auto main = new DefineFunction("main", new ValueTypeSpecByName("int"),
      VariableDefinitionArray(), mainBody);

  // Create the module.
  auto module = new Module("TestBinaryOperators");
  module->AppendExpression(main);
  auto program = new Program();
  program->AddModule(module);
  std::cout << program->Compile() << std::endl;
  std::cout << "TestBinaryOperators succeeded." << std::endl;
  delete program;

  LlvmContainer::Finalize();

  return true;
}

bool RunAllBinaryOperatorsTests()
{
  auto ret = true;
  if (!TestAssignmentOperator()) ret = false;
  if (!TestBinaryOperators()) ret = false;

  return ret;
}

} } // namespace
