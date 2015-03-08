/**
* @file Scg/Types/FloatType.h
*
* @copyright Copyright (C) 2014 Rafid Khalid Abdullah
*
* @license This file is released under Alusus Public License, Version 1.0.
* For details on usage and copying conditions read the full license in the
* accompanying license file or at <http://alusus.net/alusus_license_1_0>.
*/
//==============================================================================

#ifndef __FloatType_h__
#define __FloatType_h__

// Scg header files
#include <Types/ValueType.h>
#include <Types/ValueTypeSpec.h>

// LLVM forward declarations
#include <llvm_fwd.h>

namespace Scg
{
//! Represents the float type.
class FloatType : public ValueType
{
  friend class LlvmContainer;

  static FloatType *s_singleton;
  ValueTypeSpecByName typeSpec;

private:
  //! Constructs a float type.
  FloatType();

  //! Class destructor.
  virtual ~FloatType()
  {
  }

protected:
  //! @copydoc ValueType::InitCastingTargets()
  virtual void InitCastingTargets() const override;

public:
  /**
  * Gets a constant value of float type.
  * @param[in] value The value of the constant.
  */
  llvm::Constant *GetLlvmConstant(float value)

      const;

  //! @copydoc ValueType::GetDefaultLLVMValue()
  virtual llvm::Constant *
  GetDefaultLLVMValue(
  )
  const
  {
    return
        GetLlvmConstant(0.0f);
  }

  //! @copydoc ValueType::GetName()
  virtual
  const std::string
  GetName()

  const
  {
    return "float";
  }

  //! @copydoc ValueType::GetValueTypeSpec()
  virtual const ValueTypeSpec *
  GetValueTypeSpec() const
  override
  {
    return &

        typeSpec;
  }

  //! @copydoc ValueType::IsEqualTo()
  virtual
  bool IsEqualTo(const
  ValueType *other) const
  {
    return dynamic_cast<const FloatType *>(other)
        !=

        nullptr;
  }

  //! @copydoc ValueType::GetImplicitCastingOperator()
  virtual CastingOperator *
      GetImplicitCastingOperator(
      const ValueType *targetType, Expression *expr) const;

  //! @copydoc ValueType::GetExplicitCastingOperator()
  virtual CastingOperator
      *
      GetExplicitCastingOperator(
      const

      ValueType *
      targetType, Expression *expr) const;

  static FloatType *Get();
};
}

#endif // __FloatType_h__
