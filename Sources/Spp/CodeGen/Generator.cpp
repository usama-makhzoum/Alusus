/**
 * @file Spp/CodeGen/Generator.cpp
 * Contains the implementation of class Spp::CodeGen::Generator.
 *
 * @copyright Copyright (C) 2019 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <https://alusus.org/alusus_license_1_0>.
 */
//==============================================================================

#include "spp.h"
#include <regex>

namespace Spp::CodeGen
{

//==============================================================================
// Initialization Functions

void Generator::initBindings()
{
  auto generation = ti_cast<Generation>(this);

  generation->generateModules = &Generator::_generateModules;
  generation->generateModule = &Generator::_generateModule;
  generation->generateModuleInit = &Generator::_generateModuleInit;
  generation->generateFunction = &Generator::_generateFunction;
  generation->generateFunctionDecl = &Generator::_generateFunctionDecl;
  generation->generateUserTypeBody = &Generator::_generateUserTypeBody;
  generation->generateVarDef = &Generator::_generateVarDef;
  generation->generateTempVar = &Generator::_generateTempVar;
  generation->generateVarInitialization = &Generator::_generateVarInitialization;
  generation->generateMemberVarInitialization = &Generator::_generateMemberVarInitialization;
  generation->generateVarDestruction = &Generator::_generateVarDestruction;
  generation->generateMemberVarDestruction = &Generator::_generateMemberVarDestruction;
  generation->registerDestructor = &Generator::_registerDestructor;
  generation->generateVarGroupDestruction = &Generator::_generateVarGroupDestruction;
  generation->generateStatements = &Generator::_generateStatements;
  generation->generateStatement = &Generator::_generateStatement;
  generation->generateExpression = &Generator::_generateExpression;
  generation->generateCast = &Generator::_generateCast;
  generation->generateFunctionCall = &Generator::_generateFunctionCall;
  generation->getGeneratedType = &Generator::_getGeneratedType;
  generation->getTypeAllocationSize = &Generator::_getTypeAllocationSize;
}


//==============================================================================
// Main Operation Functions

void Generator::prepareBuild(Core::Notices::Store *noticeStore, Bool offlineExecution)
{
  VALIDATE_NOT_NULL(noticeStore);

  this->noticeStore = noticeStore;
  this->noticeStore->clearPrefixSourceLocationStack();

  this->astHelper->prepare(this->noticeStore);
  this->typeGenerator->setNoticeStore(this->noticeStore);
  this->commandGenerator->setNoticeStore(this->noticeStore);
  this->expressionGenerator->setNoticeStore(this->noticeStore);

  this->offlineExecution = offlineExecution;
  this->expressionGenerator->setOfflineExecution(offlineExecution);
}


//==============================================================================
// Code Generation Functions

Bool Generator::_generateModules(TiObject *self, Core::Data::Ast::Scope *root, GenDeps const &deps)
{
  PREPARE_SELF(generation, Generation);

  Bool result = true;
  for (Int i = 0; i < root->getCount(); ++i) {
    auto def = ti_cast<Data::Ast::Definition>(root->getElement(i));
    if (def != 0) {
      auto module = def->getTarget().ti_cast_get<Spp::Ast::Module>();
      if (module != 0) {
        if (!generation->generateModule(module, deps)) result = false;
      }
    }
  }

  return result;
}


Bool Generator::_generateModule(TiObject *self, Spp::Ast::Module *astModule, GenDeps const &deps)
{
  PREPARE_SELF(generation, Generation);
  PREPARE_SELF(generator, Generator);
  Bool result = true;
  for (Int i = 0; i < astModule->getCount(); ++i) {
    auto obj = astModule->getElement(i);
    auto def = ti_cast<Data::Ast::Definition>(obj);
    if (def != 0) {
      auto target = def->getTarget().get();
      if (target->isDerivedFrom<Spp::Ast::Module>()) {
        if (!generation->generateModule(static_cast<Spp::Ast::Module*>(target), deps)) result = false;
      } else if (target->isDerivedFrom<Spp::Ast::Function>()) {
        if (!generation->generateFunction(static_cast<Spp::Ast::Function*>(target), deps)) result = false;
      } else if (target->isDerivedFrom<Spp::Ast::UserType>()) {
        if (!generation->generateUserTypeBody(static_cast<Spp::Ast::UserType*>(target), deps)) result = false;
      } else if (generator->getAstHelper()->isAstReference(target)) {
        // Generate global variable.
        if (!generation->generateVarDef(def, deps)) {
          result = false;
        }
      }
    } else if (obj->isDerivedFrom<Core::Data::Ast::Bridge>()) {
      if (!generator->astHelper->validateUseStatement(static_cast<Core::Data::Ast::Bridge*>(obj))) result = false;
    }
  }
  return result;
}


Bool Generator::_generateModuleInit(TiObject *self, Spp::Ast::Module *astModule, GenDeps const &deps)
{
  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);
  auto startIndex = getInitStatementsGenIndex(astModule);
  Bool retVal = true;
  Int i;
  for (i = startIndex; i < astModule->getCount(); ++i) {
    auto obj = astModule->getElement(i);
    TerminalStatement terminal;
    if (!generation->generateStatement(obj, deps, terminal)) retVal = false;
  }
  setInitStatementsGenIndex(astModule, i);
  return retVal;
}


Bool Generator::_generateFunction(TiObject *self, Spp::Ast::Function *astFunc, GenDeps const &deps)
{
  PREPARE_SELF(generator, Generator);
  auto generation = ti_cast<Generation>(generator);

  auto tgFunc = tryGetCodeGenData<TiObject>(astFunc);
  if (tgFunc == 0) {
    if (!generation->generateFunctionDecl(astFunc, deps)) return false;
    tgFunc = getCodeGenData<TiObject>(astFunc);
  }

  auto astBlock = astFunc->getBody().get();
  if (astBlock != 0 && tryGetCodeGenData<TiObject>(astBlock) == 0) {
    auto astFuncType = astFunc->getType().get();
    auto tgFuncType = tryGetCodeGenData<TiObject>(astFuncType);
    ASSERT(tgFuncType != 0);

    auto astArgs = astFuncType->getArgTypes().get();
    auto astRetTypeRef = astFuncType->getRetType().get();
    Ast::Type *astRetType = astFuncType->traceRetType(generator->astHelper);

    // Prepare the function body.
    SharedList<TiObject> tgVars;
    TioSharedPtr tgContext;
    if (!deps.tg->prepareFunctionBody(tgFunc, tgFuncType, &tgVars, tgContext)) return false;

    DestructionStack destructionStack;
    GenDeps childDeps(deps, tgContext.get(), &destructionStack);

    // Store the generated ret value reference, if needed.
    if (astRetType->hasCustomInitialization(generator->astHelper, deps.tg->getExecutionContext())) {
      setCodeGenData(astRetTypeRef, tgVars.get(0));
      tgVars.remove(0);
    }

    // Store the generated data.
    setCodeGenData(astBlock, tgContext);
    for (Int i = 0; i < tgVars.getCount(); ++i) {
      auto argType = astArgs->getElement(i);
      auto argAstType = Ast::getAstType(argType);
      auto argTgType = getCodeGenData<TiObject>(argAstType);
      TioSharedPtr argTgVar;
      if (!deps.tg->generateLocalVariable(tgContext.get(), argTgType, astArgs->getElementKey(i).c_str(), 0, argTgVar)) {
        return false;
      }
      setCodeGenData(argType, argTgVar);
      Ast::Type *argSourceAstType;
      if (argAstType->hasCustomInitialization(generator->astHelper, deps.tg->getExecutionContext())) {
        argSourceAstType = generator->astHelper->getReferenceTypeFor(argAstType);
      } else {
        argSourceAstType = argAstType;
      }
      SharedList<TiObject> initTgVals;
      PlainList<TiObject> initAstTypes;
      PlainList<TiObject> initAstNodes;
      initTgVals.clear();
      initTgVals.add(tgVars.get(i));
      initAstTypes.clear();
      initAstTypes.add(argSourceAstType);
      initAstNodes.clear();
      initAstNodes.add(argType);
      TioSharedPtr argTgVarRef;
      if (!deps.tg->generateVarReference(tgContext.get(), argTgType, argTgVar.get(), argTgVarRef)) {
        return false;
      }
      if (!generation->generateVarInitialization(
        argAstType, argTgVarRef.get(), ti_cast<Core::Data::Node>(argType),
        &initAstNodes, &initAstTypes, &initTgVals, childDeps
      )) {
        return false;
      }
    }

    // Generate the function's statements.
    TerminalStatement terminal;
    auto retVal = generation->generateStatements(astBlock, childDeps, terminal);

    // Does this function need to return a value?
    if (!generator->astHelper->isVoid(astRetType) && terminal != TerminalStatement::YES) {
      // A block could have been terminated by a block or continue statement instead of a return, but that's fine
      // since top level breaks and returns will raise an error anyway.
      generator->noticeStore->add(
        std::make_shared<Spp::Notices::MissingReturnStatementNotice>(astFunc->findSourceLocation())
      );
      return false;
    }

    // Finalize the body.
    if (!deps.tg->finishFunctionBody(tgFunc, tgFuncType, &tgVars, tgContext.get())) {
      return false;
    }

    return retVal;
  }
  return true;
}


Bool Generator::_generateFunctionDecl(TiObject *self, Spp::Ast::Function *astFunc, GenDeps const &deps)
{
  PREPARE_SELF(generator, Generator);

  auto tgFunc = tryGetCodeGenData<TiObject>(astFunc);
  if (tgFunc != 0) return true;

  // Generate function type.
  TiObject *tgFunctionType;
  if (!generator->typeGenerator->getGeneratedType(
    astFunc->getType().get(), ti_cast<Generation>(self), deps.tg, tgFunctionType, 0
  )) {
    return false;
  }

  // Generate the function object.
  Str name = generator->astHelper->getFunctionName(astFunc);
  TioSharedPtr tgFuncResult;
  if (!deps.tg->generateFunctionDecl(name.c_str(), tgFunctionType, tgFuncResult)) return false;
  setCodeGenData(astFunc, tgFuncResult);

  // TODO: Do we need these attributes?
  // if (astFunc->getBody() == 0) {
  //   llvmFunc->addFnAttr(llvm::Attribute::NoCapture);
  //   llvmFunc->addFnAttr(llvm::Attribute::NoUnwind);
  // }

  return true;
}


Bool Generator::_generateUserTypeBody(TiObject *self, Spp::Ast::UserType *astType, GenDeps const &deps)
{
  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);

  TiObject *tgType;
  if (!generator->typeGenerator->getGeneratedType(astType, generation, deps.tg, tgType, 0)) return false;
  ASSERT(tgType != 0);

  // Prepare struct members.
  auto body = astType->getBody().get();
  if (body == 0) {
    throw EXCEPTION(GenericException, S("User type missing body block."));
  }

  // Generate member functions and sub types.
  Bool result = true;
  for (Int i = 0; i < body->getCount(); ++i) {
    auto obj = body->getElement(i);
    if (obj != 0) {
      if (obj->isDerivedFrom<Core::Data::Ast::Bridge>()) {
        if (!generator->astHelper->validateUseStatement(static_cast<Core::Data::Ast::Bridge*>(obj))) result = false;
        continue;
      }
      // TODO: Generate member functions.
      // TODO: Generate subtypes.
    }
  }
  if (!result) return false;

  // Generate static members.
  for (Int i = 0; i < body->getCount(); ++i) {
    auto def = ti_cast<Data::Ast::Definition>(body->getElement(i));
    if (def != 0) {
      if (generator->astHelper->getDefinitionDomain(def) == Ast::DefinitionDomain::GLOBAL) {
        auto obj = def->getTarget().get();
        if (generator->getAstHelper()->isAstReference(obj)) {
          if (!generation->generateVarDef(def, deps)) result = false;
        } else if (obj->isDerivedFrom<Spp::Ast::Function>()) {
          if (!generation->generateFunction(static_cast<Spp::Ast::Function*>(obj), deps)) result = false;
        }
      }
    }
  }

  return true;
}


Bool Generator::_generateVarDef(TiObject *self, Core::Data::Ast::Definition *definition, GenDeps const &deps)
{
  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);

  TiObject *astVar = definition->getTarget().get();
  TiObject *tgVar = tryGetCodeGenData<TiObject>(astVar);

  if (tgVar == 0) {
    // Have we previously tried to build this var?
    if (didCodeGenFail(astVar)) return false;

    // Generate the type of the variable.
    Ast::Type *astType;
    TiObject *tgType;
    if (!generator->typeGenerator->getGeneratedType(astVar, generation, deps.tg, tgType, &astType)) {
      setCodeGenFailed(astVar, true);
      return false;
    }

    // Also generate the reference type of this type.
    Ast::Type *astRefType = generator->astHelper->getPointerTypeFor(astType);
    if (astRefType == 0) {
      throw EXCEPTION(GenericException, S("Could not find reference type for the given var type."));
    }
    TiObject *tgRefType;
    if (!generator->typeGenerator->getGeneratedType(astRefType, generation, deps.tg, tgRefType, 0)) {
      throw EXCEPTION(GenericException, S("Failed to generate pointer type for the given var type."));
    }

    Ast::setAstType(astVar, astType);

    if (generator->getAstHelper()->getDefinitionDomain(definition) == Ast::DefinitionDomain::GLOBAL) {
      // Generate a global or a static variable.
      Str name = generator->getAstHelper()->resolveNodePath(definition);
      TioSharedPtr tgGlobalVar;

      // Generate the default value.
      TioSharedPtr tgDefaultValue;
      if (generator->offlineExecution) {
        if (!generator->typeGenerator->generateDefaultValue(astType, generation, deps, tgDefaultValue)) {
          setCodeGenFailed(astVar, true);
          return false;
        }
      }
      // Create the llvm global var.
      if (!deps.tg->generateGlobalVariable(tgType, name.c_str(), tgDefaultValue.get(), tgGlobalVar)) {
        setCodeGenFailed(astVar, true);
        return false;
      }
      setCodeGenData(astVar, tgGlobalVar);

      if (!generator->offlineExecution) {
        if (generator->globalItemRepo->findItem(name.c_str()) == -1) {
          // Add an entry for the variable in the repo.
          Word size;
          if (!generator->typeGenerator->getTypeAllocationSize(astType, generation, deps.tg, size)) {
            setCodeGenFailed(astVar, true);
            return false;
          }
          generator->globalItemRepo->addItem(name.c_str(), size);
        } else {
          return true;
        }
      }

      // Should this var be initialized in the global constructor or the provided context?
      if (definition->getOwner()->getOwner() == 0 || definition->getOwner()->isDerivedFrom<Spp::Ast::Module>()) {
        // This should be initialized in the global context.

        // Initialize the variable.
        TioSharedPtr tgGlobalVarRef;
        if (!deps.tg->generateVarReference(
          deps.tgGlobalConstructionContext, tgType, tgGlobalVar.get(), tgGlobalVarRef
        )) {
          return false;
        }
        SharedList<TiObject> initTgVals;
        PlainList<TiObject> initAstTypes;
        PlainList<TiObject> initAstNodes;
        if (!generation->generateVarInitialization(
          astType, tgGlobalVarRef.get(), ti_cast<Core::Data::Node>(astVar), &initAstNodes, &initAstTypes, &initTgVals,
          GenDeps(deps, deps.tgGlobalConstructionContext, deps.globalDestructionStack)
        )) {
          return false;
        }

        generation->registerDestructor(
          ti_cast<Core::Data::Node>(astVar), astType, deps.tg->getExecutionContext(), deps.globalDestructionStack
        );
      } else {
        // Should be initialized in the local context.

        // Initialize the variable.
        TioSharedPtr tgGlobalVarRef;
        if (!deps.tg->generateVarReference(deps.tgContext, tgType, tgGlobalVar.get(), tgGlobalVarRef)) return false;
        SharedList<TiObject> initTgVals;
        PlainList<TiObject> initAstTypes;
        PlainList<TiObject> initAstNodes;
        if (!generation->generateVarInitialization(
          astType, tgGlobalVarRef.get(), ti_cast<Core::Data::Node>(astVar),
          &initAstNodes, &initAstTypes, &initTgVals, deps
        )) {
          return false;
        }

        generation->registerDestructor(
          ti_cast<Core::Data::Node>(astVar), astType, deps.tg->getExecutionContext(), deps.destructionStack
        );
      }
    } else {
      auto astBlock = Core::Data::findOwner<Core::Data::Ast::Scope>(definition);
      if (ti_cast<Ast::Type>(astBlock->getOwner()) != 0) {
        // This should never happen.
        throw EXCEPTION(GenericException, S("Unexpected error while generating variable."));
      } else {
        // Generate a local variable.

        // To avoid stack overflows we need to allocate at the function level rather than any inner block.
        auto astFuncBlock = astBlock;
        while (astFuncBlock != 0 && ti_cast<Ast::Function>(astFuncBlock->getOwner()) == 0) {
          astFuncBlock = Core::Data::findOwner<Core::Data::Ast::Scope>(astFuncBlock->getOwner());
        }
        if (astFuncBlock == 0) {
          throw EXCEPTION(GenericException, S("Unexpected error while generating variable."));
        }

        // At this point we should already have a TG context.
        auto tgFuncContext = getCodeGenData<TiObject>(astFuncBlock);

        // Create the target local var.
        TioSharedPtr tgLocalVar;
        if (!deps.tg->generateLocalVariable(tgFuncContext, tgType, definition->getName().get(), 0, tgLocalVar)) {
          setCodeGenFailed(astVar, true);
          return false;
        }
        setCodeGenData(astVar, tgLocalVar);

        // Initialize the variable.
        // TODO: Should we use default values with local variables?
        // TioSharedPtr tgDefaultValue;
        // if (!astType->isDerivedFrom<Ast::ArrayType>() && !astType->isDerivedFrom<Ast::UserType>()) {
        //   if (!generator->typeGenerator->generateDefaultValue(
        //     astType, generation, deps, tgDefaultValue
        //   )) return false;
        // }
        TioSharedPtr tgLocalVarRef;
        if (!deps.tg->generateVarReference(deps.tgContext, tgType, tgLocalVar.get(), tgLocalVarRef)) {
          return false;
        }
        SharedList<TiObject> initTgVals;
        PlainList<TiObject> initAstTypes;
        PlainList<TiObject> initAstNodes;
        if (!generation->generateVarInitialization(
          astType, tgLocalVarRef.get(), definition, &initAstNodes, &initAstTypes, &initTgVals, deps
        )) return false;

        generation->registerDestructor(
          ti_cast<Core::Data::Node>(astVar), astType, deps.tg->getExecutionContext(), deps.destructionStack
        );
      }
    }
  }

  return true;
}


Bool Generator::_generateTempVar(
  TiObject *self, Core::Data::Node *astNode, Spp::Ast::Type *astType, GenDeps const &deps, Bool initialize
) {
  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);

  TiObject *tgVar = tryGetCodeGenData<TiObject>(astNode);

  if (tgVar == 0) {
    // Generate the type of the variable.
    TiObject *tgType;
    if (!generator->typeGenerator->getGeneratedType(astType, generation, deps.tg, tgType, 0)) return false;

    // Also generate the reference type of this type.
    Ast::Type *astRefType = generator->astHelper->getPointerTypeFor(astType);
    if (astRefType == 0) {
      throw EXCEPTION(GenericException, S("Could not find reference type for the given var type."));
    }
    TiObject *tgRefType;
    if (!generator->typeGenerator->getGeneratedType(astRefType, generation, deps.tg, tgRefType, 0)) {
      throw EXCEPTION(GenericException, S("Failed to generate pointer type for the given var type."));
    }

    Ast::setAstType(astNode, astType);

    Core::Data::Ast::Definition tempDef;
    tempDef.setOwner(astNode->getOwner());

    // if (generator->getAstHelper()->getDefinitionDomain(&tempDef) == Ast::DefinitionDomain::GLOBAL) {
    //   // TODO: Is there a global temp var?
    //   // Generate a global or a static variable.
    //   Str name = generator->getTempVarName();
    //   TioSharedPtr tgGlobalVar;
    //   if (generator->offlineExecution) {
    //     // Generate the default value.
    //     TioSharedPtr tgDefaultValue;
    //     if (!generator->typeGenerator->generateDefaultValue(
    //       astType, generation, tg, 0, tgDefaultValue
    //     )) return false;
    //     // Create the llvm global var.
    //     if (!tg->generateGlobalVariable(tgType, name.c_str(), tgDefaultValue.get(), tgGlobalVar)) return false;
    //   } else {
    //     // Create the llvm global var.
    //     if (!tg->generateGlobalVariable(tgType, name.c_str(), 0, tgGlobalVar)) return false;
    //     // Add an entry for the variable in the repo.
    //     Word size;
    //     if (!generator->typeGenerator->getTypeAllocationSize(astType, generation, tg, size)) return false;
    //     generator->globalItemRepo->addItem(name.c_str(), size);
    //   }

    //   // TODO: Initialize the variable.
    //   // TODO: Add to destructor list if necessary.

    //   setCodeGenData(astNode, tgGlobalVar);
    // } else {
      auto astBlock = Core::Data::findOwner<Core::Data::Ast::Scope>(astNode);
      if (ti_cast<Ast::Type>(astBlock->getOwner()) != 0) {
        // This should never happen.
        throw EXCEPTION(GenericException, S("Unexpected error while generating variable."));
      } else {
        // Generate a local variable.

        // To avoid stack overflows we need to allocate at the function or root level rather than any inner block.
        auto astAllocBlock = astBlock;
        while (
          astAllocBlock != 0 && astAllocBlock->getOwner() != 0 && ti_cast<Ast::Function>(astAllocBlock->getOwner()) == 0
        ) {
          astAllocBlock = Core::Data::findOwner<Core::Data::Ast::Scope>(astAllocBlock->getOwner());
        }
        if (astAllocBlock == 0) throw EXCEPTION(GenericException, S("Unexpected error while generating variable."));

        // At this point we should already have a TG context.
        auto tgAllocContext = getCodeGenData<TiObject>(astAllocBlock);

        // Create the llvm local var.
        TioSharedPtr tgLocalVar;
        Str name = generator->getTempVarName();
        if (!deps.tg->generateLocalVariable(tgAllocContext, tgType, name.c_str(), 0, tgLocalVar)) return false;
        setCodeGenData(astNode, tgLocalVar);

        if (initialize) {
          // Initialize the variable.
          // TODO: Should we use default values with local variables?
          // TioSharedPtr tgDefaultValue;
          // if (!astType->isDerivedFrom<Ast::ArrayType>() && !astType->isDerivedFrom<Ast::UserType>()) {
          //   if (!generator->typeGenerator->generateDefaultValue(
          //     astType, generation, tg, tgContext, tgDefaultValue
          //   )) return false;
          // }
          auto tgContext = getCodeGenData<TiObject>(astBlock);
          TioSharedPtr tgLocalVarRef;
          if (!deps.tg->generateVarReference(tgContext, tgType, tgLocalVar.get(), tgLocalVarRef)) {
            return false;
          }
          SharedList<TiObject> initTgVals;
          PlainList<TiObject> initAstTypes;
          PlainList<TiObject> initAstNodes;
          if (!generation->generateVarInitialization(
            astType, tgLocalVar.get(), astNode, &initAstNodes, &initAstTypes, &initTgVals, deps
          )) return false;

          generation->registerDestructor(astNode, astType, deps.tg->getExecutionContext(), deps.destructionStack);
        }
      }
    // }
  }

  return true;
}


Bool Generator::_generateVarInitialization(
  TiObject *self, Spp::Ast::Type *varAstType, TiObject *tgVarRef, Core::Data::Node *astNode,
  PlainList<TiObject> *paramAstNodes, PlainList<TiObject> *paramAstTypes, SharedList<TiObject> *paramTgValues,
  GenDeps const &deps
) {
  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);

  // Do we have custom initialization?
  if (varAstType->hasCustomInitialization(generator->getAstHelper(), deps.tg->getExecutionContext())) {
    // Call automatic constructors, if any.
    auto tgAutoCtor = tryGetAutoCtor<TiObject>(varAstType);
    if (tgAutoCtor != 0) {
      PlainList<TiObject> autoTgValues({ tgVarRef });
      TioSharedPtr dummy;
      if (!deps.tg->generateFunctionCall(deps.tgContext, tgAutoCtor, &autoTgValues, dummy)) return false;
    }

    // Add `this` to parameter list.
    auto varPtrAstType = generator->getAstHelper()->getReferenceTypeFor(varAstType);
    paramAstNodes->insertElement(0, astNode);
    paramAstTypes->insertElement(0, varPtrAstType);
    paramTgValues->insertElement(0, tgVarRef);

    // Do we have constructors matching the given vars?
    static Core::Data::Ast::Identifier ref({{ S("value"), TiStr(S("~init")) }});
    Ast::CalleeLookupResult calleeResult;
    if (generator->astHelper->lookupCallee(
      &ref, varAstType, false, 0, paramAstTypes, deps.tg->getExecutionContext(), calleeResult
    )) {
      auto callee = calleeResult.stack.get(calleeResult.stack.getCount() - 1);
      // Prepare the arguments to send.
      if (!generator->getExpressionGenerator()->prepareFunctionParams(
        static_cast<Ast::Function*>(callee)->getType().get(), generation, deps,
        paramAstNodes, paramAstTypes, paramTgValues
      )) return false;
      // Call the found constructor.
      GenResult result;
      return generator->getExpressionGenerator()->generateFunctionCall(
        astNode, static_cast<Ast::Function*>(callee), paramAstTypes, paramTgValues, generation, deps, result
      );
    } else if (paramAstTypes->getCount() != 1) {
      // We have custom initialization but no constructors match the given params.
      generator->noticeStore->add(std::make_shared<Spp::Notices::NoCalleeMatchNotice>(
        Core::Data::Ast::findSourceLocation(astNode)
      ));
      return false;
    }
  } else {
    // Else do we have a single arg? If so, cast then copy.
    if (paramAstTypes->getCount() == 1) {
      // Cast the value to var type.
      auto paramAstType = generator->getAstHelper()->traceType(paramAstTypes->getElement(0));
      ASSERT(paramAstType);
      TioSharedPtr tgCastedValue;
      if (!generation->generateCast(
        deps, paramAstType, varAstType, ti_cast<Core::Data::Node>(paramAstNodes->getElement(0)),
        paramTgValues->getElement(0), true, tgCastedValue)
      ) {
        generator->noticeStore->add(std::make_shared<Spp::Notices::InvalidReturnValueNotice>(
          Core::Data::Ast::findSourceLocation(paramAstNodes->getElement(0))
        ));
        return false;
      }

      // Copy the value into the var.
      auto varTgType = getCodeGenData<TiObject>(varAstType);
      TioSharedPtr assignTgRes;
      if (!deps.tg->generateAssign(deps.tgContext, varTgType, tgCastedValue.get(), tgVarRef, assignTgRes)) {
        return false;
      }
    } else if (paramAstTypes->getCount() > 0) {
      generator->noticeStore->add(std::make_shared<Spp::Notices::NoCalleeMatchNotice>(
        Core::Data::Ast::findSourceLocation(astNode)
      ));
      return false;
    } else {
      // TODO: Else call automatic (inlined) constructors, if any?
    }
  }

  return true;
}


Bool Generator::_generateMemberVarInitialization(
  TiObject *self, TiObject *astMemberNode, GenDeps const &deps
) {
  if (deps.tgSelf == 0 || deps.astSelfType == 0) {
    throw EXCEPTION(GenericException, S("Missing self while tring to initialize object member variables."));
  }

  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);

  // Get the member generated value and type.
  auto tgMemberVar = getCodeGenData<TiObject>(astMemberNode);
  auto astMemberType = Ast::getAstType(astMemberNode);
  if (!astMemberType->hasCustomInitialization(generator->getAstHelper(), deps.tg->getExecutionContext())) {
    return true;
  }
  TiObject *tgMemberType;
  if (!generation->getGeneratedType(astMemberType, deps.tg, tgMemberType, 0)) return false;

  // Get the struct ptr TG type.
  TiObject *astSelfPtrType = generator->astHelper->getPointerTypeFor(deps.astSelfType);
  TiObject *tgStructType;
  if (!generation->getGeneratedType(astSelfPtrType, deps.tg, tgStructType, 0)) return false;

  // Generate member access.
  TioSharedPtr tgMemberVarRef;
  if (!deps.tg->generateMemberVarReference(
    deps.tgContext, tgStructType, tgMemberType, tgMemberVar, deps.tgSelf, tgMemberVarRef
  )) {
    return false;
  }

  // Initialize the member variable.
  SharedList<TiObject> initTgVals;
  PlainList<TiObject> initAstTypes;
  PlainList<TiObject> initAstNodes;
  if (!generation->generateVarInitialization(
    astMemberType, tgMemberVarRef.get(), ti_cast<Core::Data::Node>(astMemberNode),
    &initAstNodes, &initAstTypes, &initTgVals, deps
  )) {
    return false;
  }

  return true;
}


Bool Generator::_generateVarDestruction(
  TiObject *self, Spp::Ast::Type *varAstType, TiObject *tgVarRef, Core::Data::Node *astNode, GenDeps const &deps
) {
  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);

  if (!varAstType->hasCustomDestruction(generator->getAstHelper(), deps.tg->getExecutionContext())) {
    return true;
  }

  // Prepare param list.
  PlainList<TiObject> paramTgValues;
  PlainList<TiObject> paramAstTypes;
  auto ptrAstType = generator->getAstHelper()->getReferenceTypeFor(varAstType);
  paramAstTypes.insertElement(0, ptrAstType);
  paramTgValues.insertElement(0, tgVarRef);

  // Find the destructor.
  static Core::Data::Ast::Identifier ref({{ S("value"), TiStr(S("~terminate")) }});
  Ast::CalleeLookupResult calleeResult;
  if (generator->astHelper->lookupCallee(
    &ref, varAstType, false, 0, &paramAstTypes, deps.tg->getExecutionContext(), calleeResult
  )) {
    auto callee = static_cast<Ast::Function*>(calleeResult.stack.get(calleeResult.stack.getCount() - 1));
    // Call the destructor.
    GenResult result;
    if (!generator->getExpressionGenerator()->generateFunctionCall(
      astNode, callee, &paramAstTypes, &paramTgValues, generation, deps, result
    )) return false;
  }

  // Call auto destructor, if any.
  auto tgAutoDtor = tryGetAutoDtor<TiObject>(varAstType);
  if (tgAutoDtor != 0) {
    PlainList<TiObject> autoTgValues({ tgVarRef });
    TioSharedPtr dummy;
    if (!deps.tg->generateFunctionCall(deps.tgContext, tgAutoDtor, &autoTgValues, dummy)) return false;
  }

  return true;
}


Bool Generator::_generateMemberVarDestruction(
  TiObject *self, TiObject *astMemberNode, GenDeps const &deps
) {
  if (deps.tgSelf == 0 || deps.astSelfType == 0) {
    throw EXCEPTION(GenericException, S("Missing self while tring to destruct object member variables."));
  }

  PREPARE_SELF(generator, Generator);
  PREPARE_SELF(generation, Generation);

  // Get the member generated value and type.
  auto tgMemberVar = getCodeGenData<TiObject>(astMemberNode);
  auto astMemberType = Ast::getAstType(astMemberNode);
  if (!astMemberType->hasCustomDestruction(generator->getAstHelper(), deps.tg->getExecutionContext())) {
    return true;
  }
  TiObject *tgMemberType;
  if (!generation->getGeneratedType(astMemberType, deps.tg, tgMemberType, 0)) return false;

  // Get the struct ptr TG type.
  TiObject *astSelfPtrType = generator->astHelper->getPointerTypeFor(deps.astSelfType);
  TiObject *tgStructType;
  if (!generation->getGeneratedType(astSelfPtrType, deps.tg, tgStructType, 0)) return false;

  // Generate member access.
  TioSharedPtr tgMemberVarRef;
  if (!deps.tg->generateMemberVarReference(
    deps.tgContext, tgStructType, tgMemberType, tgMemberVar, deps.tgSelf, tgMemberVarRef
  )) {
    return false;
  }

  // Initialize the member variable.
  SharedList<TiObject> initTgVals;
  PlainList<TiObject> initAstTypes;
  if (!generation->generateVarDestruction(
    astMemberType, tgMemberVarRef.get(), ti_cast<Core::Data::Node>(astMemberNode), deps
  )) {
    return false;
  }

  return true;
}


void Generator::_registerDestructor(
  TiObject *self, Core::Data::Node *varAstNode, Ast::Type *astType, ExecutionContext const *ec,
  DestructionStack *destructionStack
) {
  PREPARE_SELF(generator, Generator);

  // Skip if the type has no custom destructors.
  auto astUserType = ti_cast<Ast::UserType>(astType);
  if (astUserType == 0 || !astUserType->hasCustomDestruction(generator->getAstHelper(), ec)) return;

  // Add the var node to the list.
  destructionStack->pushItem(varAstNode);
}


Bool Generator::_generateVarGroupDestruction(
  TiObject *self, GenDeps const &deps, Int index
) {
  PREPARE_SELF(generation, Generation);

  for (; index < deps.destructionStack->getItemCount(); ++index) {
    auto astNode = deps.destructionStack->getItem(index);

    TiObject *tgVar = getCodeGenData<TiObject>(astNode);
    auto astType = Ast::getAstType(astNode);
    if (tgVar == 0 || astType == 0) {
      throw EXCEPTION(GenericException, S("The provided node does not seem to be a variable."));
    }
    TioSharedPtr tgVarRef;
    if (!deps.tg->generateVarReference(deps.tgContext, astType, tgVar, tgVarRef)) {
      return false;
    }

    if (!generation->generateVarDestruction(astType, tgVarRef.get(), astNode, deps)) return false;
  }

  return true;
}


Bool Generator::_generateStatements(
  TiObject *self, Core::Data::Ast::Scope *astBlock, GenDeps const &deps, TerminalStatement &terminal
) {
  PREPARE_SELF(generation, Generation);
  Bool result = true;
  terminal = TerminalStatement::UNKNOWN;
  deps.destructionStack->pushScope();
  for (Int i = 0; i < astBlock->getCount(); ++i) {
    auto astNode = astBlock->getElement(i);
    if (terminal == TerminalStatement::YES) {
      // Unreachable code.
      PREPARE_SELF(generator, Generator);
      generator->noticeStore->add(
        std::make_shared<Spp::Notices::UnreachableCodeNotice>(Core::Data::Ast::findSourceLocation(astNode))
      );
      return false;
    }
    if (!generation->generateStatement(astNode, deps, terminal)) result = false;
  }
  if (terminal != TerminalStatement::YES) {
    if (!generation->generateVarGroupDestruction(deps, deps.destructionStack->getScopeStartIndex(-1))) result = false;
  }
  deps.destructionStack->popScope();

  return result;
}


Bool Generator::_generateStatement(
  TiObject *self, TiObject *astNode, GenDeps const &deps, TerminalStatement &terminal
) {
  PREPARE_SELF(generator, Generator);
  auto generation = ti_cast<Generation>(generator);

  terminal = TerminalStatement::NO;
  Bool retVal = true;

  if (astNode->isDerivedFrom<Core::Data::Ast::Definition>()) {
    auto def = static_cast<Core::Data::Ast::Definition*>(astNode);
    auto target = def->getTarget().get();
    if (target->isDerivedFrom<Spp::Ast::Module>()) {
      retVal = generation->generateModuleInit(static_cast<Spp::Ast::Module*>(target), deps);
    // } else if (target->isDerivedFrom<Spp::Ast::Function>()) {
    //   // TODO: Generate function.
    //   generator->noticeStore->add(
    //     std::make_shared<Spp::Notices::UnsupportedOperationNotice>(def->findSourceLocation())
    //   );
    //   retVal = false;
    // } else if (target->isDerivedFrom<Spp::Ast::UserType>()) {
    //   // TODO: Generate type.
    //   generator->noticeStore->add(
    //     std::make_shared<Spp::Notices::UnsupportedOperationNotice>(def->findSourceLocation())
    //   );
    //   retVal = false;
    } else if (generator->getAstHelper()->isAstReference(target)) {
      // Generate local variable.
      retVal = generation->generateVarDef(def, deps);
    } else {
      // TODO: Make sure the definition is a literal.
      retVal = true;
    }
  } else if (astNode->isDerivedFrom<Spp::Ast::IfStatement>()) {
    auto ifStatement = static_cast<Spp::Ast::IfStatement*>(astNode);
    retVal = generator->commandGenerator->generateIfStatement(ifStatement, generation, deps, terminal);
  } else if (astNode->isDerivedFrom<Spp::Ast::WhileStatement>()) {
    auto whileStatement = static_cast<Spp::Ast::WhileStatement*>(astNode);
    retVal = generator->commandGenerator->generateWhileStatement(whileStatement, generation, deps);
  } else if (astNode->isDerivedFrom<Spp::Ast::ForStatement>()) {
    auto forStatement = static_cast<Spp::Ast::ForStatement*>(astNode);
    retVal = generator->commandGenerator->generateForStatement(forStatement, generation, deps);
  } else if (astNode->isDerivedFrom<Spp::Ast::ContinueStatement>()) {
    terminal = TerminalStatement::YES;
    auto continueStatement = static_cast<Spp::Ast::ContinueStatement*>(astNode);
    retVal = generator->commandGenerator->generateContinueStatement(continueStatement, generation, deps);
  } else if (astNode->isDerivedFrom<Spp::Ast::BreakStatement>()) {
    terminal = TerminalStatement::YES;
    auto breakStatement = static_cast<Spp::Ast::BreakStatement*>(astNode);
    retVal = generator->commandGenerator->generateBreakStatement(breakStatement, generation, deps);
  } else if (astNode->isDerivedFrom<Spp::Ast::ReturnStatement>()) {
    terminal = TerminalStatement::YES;
    auto returnStatement = static_cast<Spp::Ast::ReturnStatement*>(astNode);
    retVal = generator->commandGenerator->generateReturnStatement(returnStatement, generation, deps);
  } else if (astNode->isDerivedFrom<Core::Data::Ast::Bridge>()) {
    retVal = generator->astHelper->validateUseStatement(static_cast<Core::Data::Ast::Bridge*>(astNode));
  } else {
    GenResult result;
    retVal = generation->generateExpression(astNode, deps, result);
  }
  return retVal;
}


Bool Generator::_generateExpression(
  TiObject *self, TiObject *astNode, GenDeps const &deps, GenResult &result
) {
  PREPARE_SELF(generator, Generator);
  return generator->expressionGenerator->generate(astNode, ti_cast<Generation>(self), deps, result);
}


Bool Generator::_generateCast(
  TiObject *self, GenDeps const &deps, Spp::Ast::Type *srcType, Spp::Ast::Type *destType,
  Core::Data::Node *astNode, TiObject *tgValue, Bool implicit, TioSharedPtr &tgCastedValue
) {
  PREPARE_SELF(generator, Generator);
  return generator->typeGenerator->generateCast(
    ti_cast<Generation>(self), deps, srcType, destType, astNode, tgValue, implicit, tgCastedValue
  );
}


Bool Generator::_generateFunctionCall(
  TiObject *self, Core::Data::Node *astNode, Spp::Ast::Function *callee,
  Containing<TiObject> *paramAstTypes, Containing<TiObject> *paramTgValues,
  GenDeps const &deps, GenResult &result
) {
  PREPARE_SELF(generator, Generator);
  return generator->expressionGenerator->generateFunctionCall(
    astNode, callee, paramAstTypes, paramTgValues, ti_cast<Generation>(self), deps, result
  );
}


Bool Generator::_getGeneratedType(
  TiObject *self, TiObject *ref, TargetGeneration *tg, TiObject *&targetTypeResult, Ast::Type **astTypeResult
) {
  PREPARE_SELF(generator, Generator);
  return generator->typeGenerator->getGeneratedType(
    ref, ti_cast<Generation>(self), tg, targetTypeResult, astTypeResult
  );
}


Bool Generator::_getTypeAllocationSize(TiObject *self, Spp::Ast::Type *astType, TargetGeneration *tg, Word &result)
{
  PREPARE_SELF(generator, Generator);
  return generator->typeGenerator->getTypeAllocationSize(astType, ti_cast<Generation>(self), tg, result);
}


//==============================================================================
// Helper Functions

Str Generator::getTempVarName()
{
  return Str("#temp") + std::to_string(this->tempVarIndex++);
}

} // namespace
