
#pragma once

#include "AdditionalRestrictionsFactory.h"
#include "AdditionalRestrictionsHIVAbstract.h"

namespace Kernel
{
    class IsHivPositive : public AdditionalRestrictionsHIVAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   IsHivPositive,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(IsHivPositive)
        DECLARE_QUERY_INTERFACE()

    public:
        IsHivPositive();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:

        YesNoType::Enum m_HasEverTested;
        YesNoType::Enum m_HasEverTestedPositive;
        YesNoType::Enum m_HasReceivedPositiveResults;
    };

    class IsOnART : public AdditionalRestrictionsHIVAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   IsOnART,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(IsOnART)
        DECLARE_QUERY_INTERFACE()

    public:
        IsOnART();
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;
    };

    class HasBeenOnArtMoreOrLessThanNumMonths : public AdditionalRestrictionsHIVAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   HasBeenOnArtMoreOrLessThanNumMonths,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(HasBeenOnArtMoreOrLessThanNumMonths)
        DECLARE_QUERY_INTERFACE()

    public:
        HasBeenOnArtMoreOrLessThanNumMonths();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        float m_NumMonths;
        MoreOrLessType::Enum m_MoreOrLess;
    };

    class HasCd4BetweenMinAndMax : public AdditionalRestrictionsHIVAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   HasCd4BetweenMinAndMax,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(HasCd4BetweenMinAndMax)
        DECLARE_QUERY_INTERFACE()

    public:
        HasCd4BetweenMinAndMax();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        float m_MinCD4;
        float m_MaxCD4;
    };
}