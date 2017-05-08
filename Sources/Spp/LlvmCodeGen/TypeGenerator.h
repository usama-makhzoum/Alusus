/**
 * @file Spp/LlvmCodeGen/TypeGenerator.h
 * Contains the header of class Spp::LlvmCodeGen::TypeGenerator.
 *
 * @copyright Copyright (C) 2017 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <http://alusus.net/alusus_license_1_0>.
 */
//==============================================================================

#ifndef SPP_LLVMCODEGEN_TYPEGENERATOR_H
#define SPP_LLVMCODEGEN_TYPEGENERATOR_H

namespace Spp { namespace LlvmCodeGen
{

class TypeGenerator : public TiObject, public virtual DynamicBindings, public virtual DynamicInterfaces
{
  //============================================================================
  // Type Info

  TYPE_INFO(TypeGenerator, TiObject, "Spp.LlvmCodeGen", "Spp", "alusus.net", (
    INHERITANCE_INTERFACES(DynamicBindings, DynamicInterfaces),
    OBJECT_INTERFACE_LIST(interfaceList)
  ));


  //============================================================================
  // Implementations

  IMPLEMENT_DYNAMIC_BINDINGS(bindingMap);
  IMPLEMENT_DYNAMIC_INTERFACES(interfaceList);


  //============================================================================
  // Member Variables

  private: Core::Data::Seeker *seeker;

  private: NodePathResolver *nodePathResolver;


  //============================================================================
  // Constructors & Destructor

  public: TypeGenerator(Core::Data::Seeker *s, NodePathResolver *r) : seeker(s), nodePathResolver(r)
  {
    this->initBindingCaches();
    this->initBindings();
  }

  public: TypeGenerator(TypeGenerator *parent)
  {
    this->initBindingCaches();
    this->inheritBindings(parent);
    this->inheritInterfaces(parent);
    this->seeker = parent->getSeeker();
    this->nodePathResolver = parent->getNodePathResolver();
  }

  public: virtual ~TypeGenerator()
  {
  }


  //============================================================================
  // Member Functions

  /// @name Initialization Functions
  /// @{

  private: void initBindingCaches();
  private: void initBindings();

  public: Core::Data::Seeker* getSeeker() const
  {
    return this->seeker;
  }

  public: NodePathResolver* getNodePathResolver() const
  {
    return this->nodePathResolver;
  }

  /// @}

  /// @name Main Operation Functions
  /// @{

  public: Spp::Ast::Type* getGeneratedType(TiObject *ref, llvm::Module *llvmModule);
  public: llvm::Type* getGeneratedLlvmType(TiObject *ref, llvm::Module *llvmModule);

  /// @}

  /// @name Code Generation Functions
  /// @{

  public: METHOD_BINDING_CACHE(generateType, void, (Spp::Ast::Type*, llvm::Module*));
  public: METHOD_BINDING_CACHE(generateIntegerType, void, (Spp::Ast::IntegerType*, llvm::Module*));
  public: METHOD_BINDING_CACHE(generateFloatType, void, (Spp::Ast::FloatType*, llvm::Module*));
  public: METHOD_BINDING_CACHE(generatePointerType, void, (Spp::Ast::PointerType*, llvm::Module*));
  // public: METHOD_BINDING_CACHE(generateArrayType, void, (Spp::Ast::ArrayType*, llvm::Module*));
  // public: METHOD_BINDING_CACHE(generateStructType, void, (Spp::Ast::StructType*, llvm::Module*));

  private: static void _generateType(TiObject *self, Spp::Ast::Type *astType, llvm::Module *llvmModule);
  private: static void _generateIntegerType(TiObject *self, Spp::Ast::IntegerType *astType, llvm::Module *llvmModule);
  private: static void _generateFloatType(TiObject *self, Spp::Ast::FloatType *astType, llvm::Module *llvmModule);
  private: static void _generatePointerType(TiObject *self, Spp::Ast::PointerType *astType, llvm::Module *llvmModule);
  // private: static void _generateArrayType(TiObject *self, Spp::Ast::ArrayType *astType, llvm::Module *llvmModule);
  // private: static void _generateStructType(TiObject *self, Spp::Ast::StructType *astType, llvm::Module *llvmModule);

  /// @}

}; // class

} } // namespace

#endif
