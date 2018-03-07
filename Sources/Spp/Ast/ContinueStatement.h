/**
 * @file Spp/Ast/ContinueStatement.h
 * Contains the header of class Spp::Ast::ContinueStatement.
 *
 * @copyright Copyright (C) 2018 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <http://alusus.net/alusus_license_1_0>.
 */
//==============================================================================

#ifndef SPP_AST_CONTINUESTATEMENT_H
#define SPP_AST_CONTINUESTATEMENT_H

namespace Spp { namespace Ast
{

class ContinueStatement : public Core::Data::Node,
                          public virtual Core::Basic::Bindings, public virtual Core::Data::MapContainer,
                          public virtual Core::Data::Ast::Metadata, public virtual Core::Data::Clonable,
                          public virtual Core::Data::Printable
{
  //============================================================================
  // Type Info

  TYPE_INFO(ContinueStatement, Core::Data::Node, "Spp.Ast", "Spp", "alusus.net");
  IMPLEMENT_INTERFACES(Core::Data::Node, Core::Basic::Bindings, Core::Data::MapContainer,
                                         Core::Data::Ast::Metadata, Core::Data::Clonable,
                                         Core::Data::Printable);


  //============================================================================
  // Member Variables

  private: SharedPtr<Core::Data::Ast::IntegerLiteral> steps;


  //============================================================================
  // Implementations

  IMPLEMENT_METADATA(ContinueStatement);

  IMPLEMENT_BINDINGS(Bindings,
    (prodId, TiWord, VALUE, setProdId(value), &prodId),
    (sourceLocation, Core::Data::SourceLocation, SHARED_REF, setSourceLocation(value), sourceLocation.get())
  );

  IMPLEMENT_MAP_CONTAINER(MapContainer, (Core::Data::Ast::IntegerLiteral, steps));

  IMPLEMENT_AST_CLONABLE(ContinueStatement);

  IMPLEMENT_AST_MAP_PRINTABLE(ContinueStatement);


  //============================================================================
  // Constructors & Destructor

  IMPLEMENT_EMPTY_CONSTRUCTOR(ContinueStatement);

  IMPLEMENT_ATTR_CONSTRUCTOR(ContinueStatement);

  IMPLEMENT_ATTR_MAP_CONSTRUCTOR(ContinueStatement);

  public: virtual ~ContinueStatement()
  {
    DISOWN_SHAREDPTR(this->steps);
  }


  //============================================================================
  // Member Functions

  public: void setSteps(SharedPtr<Core::Data::Ast::IntegerLiteral> const &s)
  {
    UPDATE_OWNED_SHAREDPTR(this->steps, s);
  }

  public: SharedPtr<Core::Data::Ast::IntegerLiteral> const& getSteps() const
  {
    return this->steps;
  }

}; // class

} } // namespace

#endif
