/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Drugs.h"

namespace Kernel
{
    struct IHIVDrugEffectsApply;

    class ARTBasic : public GenericDrug
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ARTBasic, IDistributableIntervention);

    public:
        bool Configure( const Configuration * );
        ARTBasic();
        virtual ~ARTBasic();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );

        virtual std::string GetDrugName();

    protected:
        // These have same names as analogous methods on container but are internal for the drug itself.

        bool viral_suppression;
        float days_to_achieve_suppression ;

        virtual void ApplyEffects();

        IHIVDrugEffectsApply * itbda;

    private:

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, ARTBasic& drug, const unsigned int v);
#endif
    };
}
