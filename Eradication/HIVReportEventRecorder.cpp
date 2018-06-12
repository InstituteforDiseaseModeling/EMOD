/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

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

SETUP_LOGGING( "HIVReportEventRecorder" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(HIVReportEventRecorder,HIVReportEventRecorder)

    IReport* HIVReportEventRecorder::CreateReport()
    {
        return new HIVReportEventRecorder();
    }

    HIVReportEventRecorder::HIVReportEventRecorder()
        : STIReportEventRecorder()
        , m_InterventionStatusKey()
    {
    }

    HIVReportEventRecorder::~HIVReportEventRecorder()
    {
    }

    void HIVReportEventRecorder::Initialize( unsigned int nrmSize )
    {
        ReportEventRecorder::Initialize( nrmSize );

        // has to be done if Initialize() since it is called after the demographics is read
        IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( "InterventionStatus", "", false );
        if( p_ip != nullptr )
        {
            m_InterventionStatusKey = p_ip->GetKey<IPKey>();
        }
    }

    std::string HIVReportEventRecorder::GetHeader() const
    {
        std::string header = ReportEventRecorder::GetHeader() ;
        header += "," ;
        header += "HasHIV," ;
        header += "OnART," ;
        header += "CD4," ;
        header += "WHO_Stage," ;
        header += "InterventionStatus";

        return header;
    }

    std::string HIVReportEventRecorder::GetOtherData( IIndividualHumanEventContext *context, 
                                                      const EventTrigger& trigger )
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

        std::string intervention_state = "None";
        if( m_InterventionStatusKey.IsValid() )
        {
            intervention_state = context->GetProperties()->Get( m_InterventionStatusKey ).GetValueAsString();
        }

        std::stringstream ss ;
        ss                  << ","
           << has_hiv       << ","
           << on_ART        << ","
           << cd4           << ","
           << who_stage     << ","
           << intervention_state;

        return ss.str() ;
    }
}
