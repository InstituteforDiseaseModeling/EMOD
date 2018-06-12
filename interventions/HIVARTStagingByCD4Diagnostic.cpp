/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVARTStagingByCD4Diagnostic.h"

#include "InfectionHIV.h"
#include "IIndividualHumanHIV.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVMedicalHistory

SETUP_LOGGING( "HIVARTStagingByCD4Diagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVARTStagingByCD4Diagnostic, HIVARTStagingAbstract)
    END_QUERY_INTERFACE_DERIVED(HIVARTStagingByCD4Diagnostic, HIVARTStagingAbstract)

    IMPLEMENT_FACTORY_REGISTERED(HIVARTStagingByCD4Diagnostic)

    HIVARTStagingByCD4Diagnostic::HIVARTStagingByCD4Diagnostic()
    : HIVARTStagingAbstract()
    {
    }

    HIVARTStagingByCD4Diagnostic::HIVARTStagingByCD4Diagnostic( const HIVARTStagingByCD4Diagnostic& master )
        : HIVARTStagingAbstract( master )
    {
        threshold = master.threshold;
        ifActiveTB = master.ifActiveTB;
        ifPregnant = master.ifPregnant;
    }

    bool HIVARTStagingByCD4Diagnostic::Configure( const Configuration* inputJson )
    {
        initConfigComplexType("Threshold",    &threshold,  HIV_ASBCD_Threshold_DESC_TEXT    );
        initConfigComplexType("If_Active_TB", &ifActiveTB, HIV_ASBCD_If_Active_TB_DESC_TEXT );
        initConfigComplexType("If_Pregnant",  &ifPregnant, HIV_ASBCD_If_Pregnant_DESC_TEXT  );

        return HIVARTStagingAbstract::Configure( inputJson );
    }

    bool HIVARTStagingByCD4Diagnostic::positiveTestResult( IIndividualHumanHIV *pHIV, 
                                                           float year, 
                                                           float CD4count, 
                                                           bool hasActiveTB, 
                                                           bool isPregnant )
    {
        if( !pHIV->HasHIV() ) {
            return false;
        }

        bool result =  (                CD4count <= threshold.getValuePiecewiseConstant( year, -FLT_MAX) )
                    || ( hasActiveTB && CD4count <= ifActiveTB.getValuePiecewiseConstant(year, -FLT_MAX) )
                    || ( isPregnant  && CD4count <= ifPregnant.getValuePiecewiseConstant(year, -FLT_MAX) );
        return result ;
    }

    REGISTER_SERIALIZABLE(HIVARTStagingByCD4Diagnostic);

    void HIVARTStagingByCD4Diagnostic::serialize(IArchive& ar, HIVARTStagingByCD4Diagnostic* obj)
    {
        HIVARTStagingAbstract::serialize( ar, obj );
        HIVARTStagingByCD4Diagnostic& diag = *obj;
        ar.labelElement("threshold" ) & diag.threshold;
        ar.labelElement("ifActiveTB") & diag.ifActiveTB;
        ar.labelElement("ifPregnant") & diag.ifPregnant;
    }
}
