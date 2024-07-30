
#pragma once

#include "AdditionalRestrictionsFactory.h"
#include "AdditionalRestrictionsSTIAbstract.h"

namespace Kernel
{
    ENUM_DEFINE( NumMonthsType,
        ENUM_VALUE_SPEC( THREE_MONTHS  , 0 )
        ENUM_VALUE_SPEC( SIX_MONTHS    , 1 )
        ENUM_VALUE_SPEC( NINE_MONTHS   , 2 )
        ENUM_VALUE_SPEC( TWELVE_MONTHS , 3 ) )

    class IsCircumcised : public AdditionalRestrictionsSTIAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   IsCircumcised,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(IsCircumcised)
        DECLARE_QUERY_INTERFACE()

    public:
        IsCircumcised();
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;
    };

    class IsPostDebut : public AdditionalRestrictionsSTIAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   IsPostDebut,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(IsPostDebut)
        DECLARE_QUERY_INTERFACE()

    public:
        IsPostDebut();
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;
    };

    class HasMoreOrLessThanNumPartners : public AdditionalRestrictionsSTIAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   HasMoreOrLessThanNumPartners,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(HasMoreOrLessThanNumPartners)
        DECLARE_QUERY_INTERFACE()

    public:
        HasMoreOrLessThanNumPartners();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        int m_NumPartners;
        MoreOrLessType::Enum m_MoreOrLess;
        TargetRelationshipType::Enum m_OfRelationshipType;
    };

    class HasHadMultiplePartnersInLastNumMonths : public AdditionalRestrictionsSTIAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   HasHadMultiplePartnersInLastNumMonths,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(HasHadMultiplePartnersInLastNumMonths)
        DECLARE_QUERY_INTERFACE()

    public:
        HasHadMultiplePartnersInLastNumMonths();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        NumMonthsType::Enum m_NumMonths;
        TargetRelationshipType::Enum m_OfRelationshipType;
    };

}