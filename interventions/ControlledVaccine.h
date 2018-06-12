/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Vaccine.h"
#include "EventTrigger.h"

namespace Kernel
{
    struct IControlledVaccine : ISupports
    {
        virtual const std::string& GetInterventionName() const = 0;
        virtual bool AllowRevaccination( const IControlledVaccine& rNewVaccine ) const = 0;
    };

    class ControlledVaccine : public SimpleVaccine, public IControlledVaccine
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ControlledVaccine, IDistributableIntervention)
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        ControlledVaccine();
        ControlledVaccine( const ControlledVaccine& );
        virtual ~ControlledVaccine();


        // SimpleVaccine
        virtual bool Configure( const Configuration* pConfig ) override;
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void Update(float dt) override;

    protected:
        virtual const std::string& GetInterventionName() const override;
        virtual bool AllowRevaccination( const IControlledVaccine& rNewVaccine ) const override;

        float        m_DurationToWaitBeforeRevaccination;
        float        m_TimeSinceVaccination;
        EventTrigger m_DistributedEventTrigger;
        EventTrigger m_ExpiredEventTrigger;

        DECLARE_SERIALIZABLE(ControlledVaccine);
    };
}
