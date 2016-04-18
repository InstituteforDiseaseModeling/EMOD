/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVSetCascadeState.h"

static const char * _module = "HIVSetCascadeState";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVSetCascadeState, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVSetCascadeState, HIVSimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVSetCascadeState)

    HIVSetCascadeState::HIVSetCascadeState()
    : HIVSimpleDiagnostic(  )
    {
    }

    HIVSetCascadeState::HIVSetCascadeState( const HIVSetCascadeState& master )
    : HIVSimpleDiagnostic( master )
    {
    }

    bool
    HIVSetCascadeState::Configure(const Configuration* inputJson)
    {
        bool ret = HIVSimpleDiagnostic::Configure( inputJson );
        if( (negative_diagnosis_event != NO_TRIGGER_STR) && !negative_diagnosis_event.IsUninitialized() )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "HIVSetCascadeState can't have a Negative_Diagnosis_Event." );
        }

        return ret;
    }

    bool HIVSetCascadeState::positiveTestResult()
    {
        return true;
    }

    REGISTER_SERIALIZABLE(HIVSetCascadeState);

    void HIVSetCascadeState::serialize(IArchive& ar, HIVSetCascadeState* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVSetCascadeState& cascade = *obj;

        //ar.labelElement("xxx") & cascade.xxx;
    }
}
