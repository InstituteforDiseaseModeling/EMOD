/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "HIVReportEventRecorder.h"
#include "Log.h"
#include "Exceptions.h"
#include "NodeEventContext.h"
#include "IndividualEventContext.h"
#include "Contexts.h"
#include "FileSystem.h"
#include "IIndividualHumanHIV.h"
#include "SimulationEnums.h"
#include "IIndividualHumanHIV.h"
#include "InfectionHIV.h"
#include "IHIVInterventionsContainer.h"
#include "SusceptibilityHIV.h"

static const char* _module = "HIVReportEventRecorder";

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(HIVReportEventRecorder,HIVReportEventRecorder)

    IReport* HIVReportEventRecorder::CreateReport()
    {
        return new HIVReportEventRecorder();
    }

    HIVReportEventRecorder::HIVReportEventRecorder()
        : ReportEventRecorder()
    {
    }

    HIVReportEventRecorder::~HIVReportEventRecorder()
    {
    }

    std::string HIVReportEventRecorder::GetHeader() const
    {
        std::string header = ReportEventRecorder::GetHeader() ;
        header += "," ;
        header += "HasHIV," ;
        header += "OnART," ;
        header += "CD4," ;
        header += "WHO_Stage," ;
        header += "CascadeState";

        return header;
    }

    std::string HIVReportEventRecorder::GetOtherData( IIndividualHumanEventContext *context, 
                                                      const std::string& StateChange )
    {
        IIndividualHumanHIV * iindividual_hiv = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&iindividual_hiv) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanHIV", "IIndividualHumanEventContext");
        }
        const char  has_hiv    = iindividual_hiv->HasHIV() ? 'Y' : 'N' ;

        const char  on_ART = iindividual_hiv->GetHIVInterventionsContainer()->OnArtQuery() ? 'Y' : 'N' ;

        float cd4 = 1e6;
        float who_stage = -1.0f;
        if( iindividual_hiv->HasHIV() )
        {
            cd4       = iindividual_hiv->GetHIVSusceptibility()->GetCD4count();
            who_stage = iindividual_hiv->GetHIVInfection()->GetWHOStage();
        }

        IHIVCascadeOfCare *ihcc = nullptr;
        if ( s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHIVCascadeOfCare), (void **)&ihcc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "IHIVCascadeOfCare", "IIndividualHumanEventContext" );
        }

        std::string cascade_state = ihcc->getCascadeState();

        if( cascade_state.length() == 0 )
        {
            cascade_state = "None";
        }

        std::stringstream ss ;
        ss                   << ","
           << has_hiv       << ","
           << on_ART        << ","
           << cd4           << ","
           << who_stage     << ","
           << cascade_state;

        return ss.str() ;
    }
}
