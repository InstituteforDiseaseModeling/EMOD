
#pragma once

#include "AdditionalRestrictionsFactory.h"
#include "AdditionalRestrictionsAbstract.h"
#include "Properties.h"
#include "InterventionName.h"

namespace Kernel
{
    class HasIntervention : public AdditionalRestrictionsAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   HasIntervention,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(HasIntervention)
        DECLARE_QUERY_INTERFACE()

    public:
        HasIntervention();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        InterventionName m_InterventionName;
    };

    class HasIP : public AdditionalRestrictionsAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   HasIP,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(HasIP)
        DECLARE_QUERY_INTERFACE()

    public:
        HasIP();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
        IPKeyValue m_IPKeyValue;
    };

    class IsPregnant : public AdditionalRestrictionsAbstract
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   IsPregnant,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(IsPregnant)
        DECLARE_QUERY_INTERFACE()

    public:
        IsPregnant();
        virtual bool Configure(const Configuration* config) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;

    private:
    };
}