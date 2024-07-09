
#pragma once

#include "Configure.h"
#include "ISerializable.h"
#include "AdditionalRestrictionsSTIAbstract.h"
#include "AdditionalRestrictionsFactory.h"
#include "IRelationship.h"

namespace Kernel
{
    ENUM_DEFINE( RecentlyType,
        ENUM_VALUE_SPEC( NA      , -1 )
        ENUM_VALUE_SPEC( STARTED ,  0 )
        ENUM_VALUE_SPEC( ENDED   ,  1 ) )

    struct IRelationship;

    class HasRelationship : public AdditionalRestrictionsSTIAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   HasRelationship,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(HasRelationship)
        DECLARE_QUERY_INTERFACE()

    public:
        HasRelationship();
        virtual ~HasRelationship();

        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        bool IsRelationshipQualified( IIndividualHumanSTI* pHumanSti,
                                      IRelationship* pRel ) const;

        TargetRelationshipType::Enum m_OfRelationshipType;
        RecentlyType::Enum m_ThatRecently;
        RelationshipTerminationReason::Enum m_ThatRecentlyEndedDueTo;
        IAdditionalRestrictions* m_pWithPartnerWho;
        float m_TimeStepDuration;
    };
}