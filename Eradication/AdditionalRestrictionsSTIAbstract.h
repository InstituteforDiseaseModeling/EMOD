
#pragma once

#include "RelationshipType.h"
#include "AdditionalRestrictionsAbstract.h"

namespace Kernel
{
    ENUM_DEFINE( TargetRelationshipType,
        ENUM_VALUE_SPEC( NA         , -1 )
        ENUM_VALUE_SPEC( TRANSITORY , RelationshipType::TRANSITORY )
        ENUM_VALUE_SPEC( INFORMAL   , RelationshipType::INFORMAL   )
        ENUM_VALUE_SPEC( MARITAL    , RelationshipType::MARITAL    )
        ENUM_VALUE_SPEC( COMMERCIAL , RelationshipType::COMMERCIAL ) )

    ENUM_DEFINE( YesNoType,
        ENUM_VALUE_SPEC( NA  , 0 )
        ENUM_VALUE_SPEC( NO  , 1 )
        ENUM_VALUE_SPEC( YES , 2 ) )

    struct IIndividualHumanSTI;

    class AdditionalRestrictionsSTIAbstract : public AdditionalRestrictionsAbstract
    {
    public:
        AdditionalRestrictionsSTIAbstract();

    protected:
        IIndividualHumanSTI * GetSTIIndividual(IIndividualHumanEventContext* pContext) const;
    };
}
