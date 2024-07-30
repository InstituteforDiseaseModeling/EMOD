
#pragma once

#include "ISupports.h"

namespace Kernel
{
    class StrainIdentityMalariaCoTran;

    struct INodeMalariaCoTransmission : public ISupports
    {
        virtual const StrainIdentityMalariaCoTran& GetCoTranStrainIdentityForPerson( bool isIndoor, uint32_t personID ) const = 0;
        virtual const StrainIdentityMalariaCoTran& GetCoTranStrainIdentityForVector( bool isIndoor, uint32_t vectorID ) const = 0;
        virtual void VectorBitPerson( bool isIndoor, uint32_t vectorID ) = 0;
    };

    struct IMalariaHumanReport : public ISupports
    {
        virtual const StrainIdentityMalariaCoTran& GetRecentTransmissionInfo() const = 0;
    };
}
