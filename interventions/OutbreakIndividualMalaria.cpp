/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "OutbreakIndividualMalaria.h"
#include "Exceptions.h"
#include "SimulationConfig.h"
#include "IGenomeMarkers.h"
#include "MalariaParameters.h"

SETUP_LOGGING( "OutbreakIndividualMalaria" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( OutbreakIndividualMalaria, OutbreakIndividual )
    END_QUERY_INTERFACE_DERIVED( OutbreakIndividualMalaria, OutbreakIndividual )

    IMPLEMENT_FACTORY_REGISTERED( OutbreakIndividualMalaria )

    OutbreakIndividualMalaria::OutbreakIndividualMalaria()
        : OutbreakIndividual()
        , m_GenomeMarkerNames()
        , m_CreateRandomGenome(false)
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    OutbreakIndividualMalaria::~OutbreakIndividualMalaria()
    {
    }

    bool OutbreakIndividualMalaria::Configure( const Configuration * inputJson )
    {
        bool ret = OutbreakIndividual::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            genome = -1;  // tell OutbreakIndividual::GetNewStrainIdentity() to create random genome
            if(!m_CreateRandomGenome )
            {
                genome = GET_CONFIGURABLE( SimulationConfig )->malaria_params->pGenomeMarkers->CreateBits( m_GenomeMarkerNames );
            }
        }
        return ret;
    }

    void OutbreakIndividualMalaria::ConfigureGenome( const Configuration * inputJson )
    {
        const std::set<std::string>* p_known_markers = nullptr;
        if( !JsonConfigurable::_dryrun )
        {
            p_known_markers = &(GET_CONFIGURABLE( SimulationConfig )->malaria_params->pGenomeMarkers->GetNameSet());
        }

        initConfigTypeMap( "Create_Random_Genome", &m_CreateRandomGenome, OIM_Create_Random_Genome_DESC_TEXT, false );
        initConfigTypeMap( "Genome_Markers", &m_GenomeMarkerNames, OIM_Genome_Markers_DESC_TEXT, "<configuration>.Genome_Markers", *p_known_markers );
    }
}
