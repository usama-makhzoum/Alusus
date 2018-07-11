/**
 * @file Spp/Ast/IntegerType.cpp
 * Contains the implementation of class Spp::Ast::IntegerType.
 *
 * @copyright Copyright (C) 2018 Sarmad Khalid Abdullah
 *
 * @license This file is released under Alusus Public License, Version 1.0.
 * For details on usage and copying conditions read the full license in the
 * accompanying license file or at <http://alusus.net/alusus_license_1_0>.
 */
//==============================================================================

#include "spp.h"

namespace Spp { namespace Ast
{

//==============================================================================
// Member Functions

Word IntegerType::getBitCount(Helper *helper) const
{
  if (this->bitCountRef == 0) {
    this->bitCountRef = helper->getRootManager()->parseExpression(STR("bitCount"));
  }
  auto bitCount = ti_cast<TiInt>(
    helper->getSeeker()->doGet(this->bitCountRef.get(), this->getOwner())
  );
  if (bitCount == 0) {
    throw EXCEPTION(GenericException, STR("Could not find bitCount value."));
  }
  return bitCount->get();
}


TypeMatchStatus IntegerType::matchTargetType(Type const *type, Helper *helper, ExecutionContext const *ec) const
{
  if (this == type) return TypeMatchStatus::EXACT;
  else {
    auto integerType = ti_cast<IntegerType const>(type);
    if (integerType != 0) {
      auto thisBitCount = this->getBitCount(helper);
      auto targetBitCount = integerType->getBitCount(helper);
      if (thisBitCount == targetBitCount && this->isSigned() == integerType->isSigned()) {
        return TypeMatchStatus::EXACT;
      } else if (targetBitCount == 1 && thisBitCount == 1) {
        return TypeMatchStatus::PROMOTION;
      } else if (integerType->isSigned() == this->isSigned() && targetBitCount > thisBitCount) {
        return TypeMatchStatus::PROMOTION;
      } else if (integerType->isSigned() && targetBitCount > thisBitCount) {
        return TypeMatchStatus::PROMOTION;
      } else if (thisBitCount > 1 && targetBitCount == 1) {
        return TypeMatchStatus::NONE;
      } else {
        return TypeMatchStatus::IMPLICIT_CAST;
      }
    } else if (type->isDerivedFrom<FloatType>()) {
      return TypeMatchStatus::IMPLICIT_CAST;
    } else if (type->isDerivedFrom<PointerType>() && ec->getPointerBitCount() == this->getBitCount(helper)) {
      return TypeMatchStatus::EXPLICIT_CAST;
    } else {
      return TypeMatchStatus::NONE;
    }
  }
}

} } // namespace
