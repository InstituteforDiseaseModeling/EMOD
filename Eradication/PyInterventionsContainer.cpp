/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#ifdef ENABLE_PYTHON

//#include "SimpleTypemapRegistration.h"
#include "Sugar.h"
#include "Debug.h"
#include "Environment.h"

#include "Drugs.h"

#include "Contexts.h"
#include "InterventionFactory.h"
#include "PyInterventionsContainer.h"


namespace Kernel
{
    static const char* _module = "PyInterventionsContainer";

    BEGIN_QUERY_INTERFACE_DERIVED(PyInterventionsContainer, InterventionsContainer)
        //HANDLE_INTERFACE(IPyVaccineEffects)
        //HANDLE_INTERFACE(IPyDrugEffects)
        HANDLE_INTERFACE(IPyDrugEffectsApply)
    END_QUERY_INTERFACE_DERIVED(PyInterventionsContainer, InterventionsContainer)

    PyInterventionsContainer::PyInterventionsContainer()
    {
    }

    PyInterventionsContainer::~PyInterventionsContainer()
    {
    }

    void PyInterventionsContainer::Update(float dt)
    {
        // call base level
        InterventionsContainer::Update(dt);
    }

    bool PyInterventionsContainer::GiveIntervention( IDistributableIntervention * pIV )
    {
        pIV->SetContextTo( parent );

        // Additionally, keep this newly-distributed polio-vaccine pointer in the 'new_vaccines' list.  
        // IndividualHuman::applyNewInterventionEffects will come looking for these to apply to SusceptibilityPy.
        IDistributableIntervention* ipvac = nullptr;
/*
        IDrug * pDrug = NULL;
        if( s_OK == pIV->QueryInterface(GET_IID(IDrug), (void**) &pDrug) )
        {
            LOG_DEBUG("Getting a drug\n");
            GiveDrug( pDrug );
        } 
*/
        return InterventionsContainer::GiveIntervention(pIV);
    }

    void PyInterventionsContainer::GiveDrug(IDrug* drug)
    {
        drug->ConfigureDrugTreatment();
    }

    void PyInterventionsContainer::ApplyDrugVaccineReducedAcquireEffect( float rate )
    {
    }
    
    void PyInterventionsContainer::ApplyDrugVaccineReducedTransmitEffect( float rate )
    {
    }

}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::PyInterventionsContainer)
namespace Kernel
{
    template<class Archive>
    void serialize(Archive &ar, PyInterventionsContainer& cont, const unsigned int v)
    {
        static const char * _module = "PyInterventionsContainer";
        LOG_DEBUG("(De)serializing PyInterventionsContainer\n");

        //ar & cont.new_vaccines;  // SusceptibilityPy update based on new vaccines done in same time step?
        boost::serialization::void_cast_register<PyInterventionsContainer, InterventionsContainer>();
        ar & boost::serialization::base_object<Kernel::InterventionsContainer>(cont);
    }
}
#endif

#endif // ENABLE_PYTHON
