
#pragma once

#include "Configure.h"
#include "ISerializable.h"
#include "IAdditionalRestrictions.h"
#include "AdditionalRestrictionsFactory.h"

namespace Kernel
{
    class TargetingLogic : public IAdditionalRestrictions
                         , public JsonConfigurable
                         , public IComplexJsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(AdditionalRestrictionsFactory,
                                   TargetingLogic,
                                   IAdditionalRestrictions)
        DECLARE_SERIALIZABLE(TargetingLogic)
        DECLARE_QUERY_INTERFACE()

    public:
        TargetingLogic();
        virtual ~TargetingLogic();

        virtual bool Configure(const Configuration* config) override;
        virtual void ConfigureFromJsonAndKey(const Configuration* inputJson, const std::string& key) override;
        virtual bool IsQualified(IIndividualHumanEventContext* pContext) const override;
        virtual json::QuickBuilder GetSchema() override;
        virtual bool  HasValidDefault() const override;

    private:
        std::vector<std::vector<IAdditionalRestrictions*>> m_Restrictions;
    };
}