/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#ifdef ENABLE_PYTHON
#include <string>
#include <list>
#include <vector>

#include "Drugs.h"
#include "Interventions.h"
#include "InterventionsContainer.h"
//#include "SimpleTypemapRegistration.h"

namespace Kernel
{
    // this container becomes a help implementation member of the relevant IndividualHuman class 
    // it needs to implement consumer interfaces for all the relevant intervention types
    struct IPyDrugEffectsApply : public ISupports
    {
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) = 0;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) = 0;
    };

    class IPyVaccine;

    class PyInterventionsContainer : public InterventionsContainer,
                                          //public IPyDrugEffects,
                                          public IPyDrugEffectsApply
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        PyInterventionsContainer();
        virtual ~PyInterventionsContainer();

        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);

        // IVaccineConsumer: not any more!
        virtual bool GiveIntervention( IDistributableIntervention * pIV );

        // IPyDrugEffectsApply
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ); // not used for anything
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ); // not used for anything

        //IPyDrugEffects(Get)

        virtual void Update(float dt); // example of intervention timestep update

    protected:
        void GiveDrug(IDrug* drug);

    private:
    };
}
#endif
