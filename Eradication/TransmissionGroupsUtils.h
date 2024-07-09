
#pragma once

#include "ITransmissionGroups.h"

namespace Kernel
{
    // This is a set of utility methods used by TransmissionGroups.
    namespace TransmissionGroupsUtils
    {
        void checkForValidValueListSize( const PropertyValueList_t& values );
        void checkForValidScalingMatrixSize( const ScalingMatrix_t& scalingMatrix,
                                             const PropertyValueList_t& values );
        void CheckForValidStrainListSize( const StrainIdentitySet_t& strains );
        void InitializeCumulativeMatrix( ScalingMatrix_t& cumulativeMatrix );
        void AggregatePropertyMatrixWithCumulativeMatrix( const ScalingMatrix_t& propertyMatrix,
                                                          ScalingMatrix_t& cumulativeMatrix );
    };
}
