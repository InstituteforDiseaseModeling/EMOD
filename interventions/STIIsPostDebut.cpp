/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "STIIsPostDebut.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "IndividualSTI.h"

static const char * _module = "STIIsPostDebut";

/*
This intervention works in both STI_SIM and HIV_SIM sim_types.
It is based on SimpleDiagnostic and therefore inherits:
- Positive_Diagnostic_Event
- Treatment_Fraction
- Base_Specificity
- Base_Sensitivity
- Cost_To_Consumer

We also add to it:
- Negative_Diagnosis_Event

The intervention can be distributed to an individual at any time. It will 
remain with them until days_to_diagnosis is 0. Then it will test if they
are post-debut. IF yes, broadcast a certain event. If not, broadcast another
event.
*/
namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(STIIsPostDebut, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(STIIsPostDebut, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(STIIsPostDebut)

    STIIsPostDebut::STIIsPostDebut()
    : SimpleDiagnostic()
    , negative_diagnosis_event()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
        initConfigTypeMap( "Negative_Diagnosis_Event", &negative_diagnosis_event, STI_IPD_Negative_Diagnosis_Event_DESC_TEXT );
    }

    STIIsPostDebut::STIIsPostDebut( const STIIsPostDebut& master )
        : SimpleDiagnostic( master )
    {
        negative_diagnosis_event = master.negative_diagnosis_event;
    }

    bool STIIsPostDebut::positiveTestResult()
    {
        IIndividualHumanSTI * sti_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanSTI", "IIndividualHumanContext" );
        }

        IIndividualHuman* ih_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHuman), (void**)&ih_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHuman", "IIndividualHumanContext" );
        }

        bool qualifies = (ih_parent->GetAge() >= sti_parent->GetDebutAge());
        LOG_DEBUG_F( "Individual %d getting tested: returning %d.\n", parent->GetSuid().data, qualifies );
        return qualifies;
    }

    void
    STIIsPostDebut::onNegativeTestResult()
    {
        auto iid = parent->GetSuid().data;
        LOG_DEBUG_F( "Individual %d tested 'negative' in STIIsPostDebut, broadcasting negative event.\n", iid );
        
        if( (negative_diagnosis_event != NO_TRIGGER_STR) && !negative_diagnosis_event.IsUninitialized() )
        {
            LOG_DEBUG_F( "Broadcasting event %s as negative diagnosis event for individual %d.\n", negative_diagnosis_event.c_str(), iid );
            broadcastEvent( negative_diagnosis_event );
        }
        else
        {
            LOG_DEBUG_F( "Negative diagnosis event is NoTrigger for individual %d.\n", iid );
        }
        expired = true;
    }

    REGISTER_SERIALIZABLE(STIIsPostDebut);

    void STIIsPostDebut::serialize(IArchive& ar, STIIsPostDebut* obj)
    {
        SimpleDiagnostic::serialize( ar, obj );
        STIIsPostDebut& debut = *obj;
        ar.labelElement("negative_diagnosis_event") & debut.negative_diagnosis_event;
    }
}
