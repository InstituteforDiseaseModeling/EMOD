/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "StrainIdentity.h"
#include "Infection.h"
#include "SimulationConfig.h"
#include "Log.h"

SETUP_LOGGING( "StrainIdentity" )

namespace Kernel {

    StrainIdentity::StrainIdentity(void)
        : antigenID(0)
        , geneticID(0)
    {
    }

    StrainIdentity::StrainIdentity(int initial_antigen, int initial_genome, RANDOMBASE * pRng )
        : antigenID( initial_antigen )
        , geneticID( initial_genome )
    {
        if( initial_antigen >= int(InfectionConfig::number_basestrains) )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "initial_genome", initial_antigen, InfectionConfig::number_basestrains );
        }

        if ( initial_genome < 0 )
        {
            int substrains = InfectionConfig::number_substrains;
            if (substrains & (substrains-1))
            {
                std::stringstream ss;
                ss << "'Number_Substrains'=" << substrains << "  Only supporting random genome generation for Number_Substrains as factor of two.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            unsigned int BARCODE_BITS = 0;
            while(substrains >>= 1) ++BARCODE_BITS;
            geneticID = pRng->ul() & ((1 << BARCODE_BITS)-1);
            //genome = pIndiv->GetRng()->uniformZeroToN16(simConfigObj->number_substrains);
            LOG_DEBUG_F("random genome generation... antigen: %d\t genome: %d\n", antigenID, geneticID);
        }
        else if( initial_genome >= int(InfectionConfig::number_substrains) )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "initial_genome", initial_genome, InfectionConfig::number_substrains );
        }
    }

    StrainIdentity::StrainIdentity( const IStrainIdentity *copy )
        : antigenID( copy->GetAntigenID() )
        , geneticID( copy->GetGeneticID() )
    {
        LOG_DEBUG_F( "New infection with antigen id %d and genetic id %d\n", antigenID, geneticID );
    }

    StrainIdentity::~StrainIdentity(void)
    {
    }

    int StrainIdentity::GetAntigenID(void) const
    {
        return antigenID;
    }

    int StrainIdentity::GetGeneticID(void) const
    {
        return geneticID;
    }

    void StrainIdentity::SetAntigenID(int in_antigenID)
    {
        antigenID = in_antigenID;
    }

    void StrainIdentity::SetGeneticID(int in_geneticID)
    {
        geneticID = in_geneticID;
    }

    void StrainIdentity::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        strainId->SetAntigenID(antigenID);
        strainId->SetGeneticID(geneticID);
    }

    IArchive& StrainIdentity::serialize(IArchive& ar, StrainIdentity*& ptr)
    {
        if (!ar.IsWriter())
        {
            ptr = new StrainIdentity();
        }

        StrainIdentity& strain = *ptr;

        serialize( ar, strain );

        return ar;
    }

    IArchive& StrainIdentity::serialize(IArchive& ar, StrainIdentity& strain)
    {
        ar.startObject();
            ar.labelElement("antigenID") & strain.antigenID;
            ar.labelElement("geneticID") & strain.geneticID;
        ar.endObject();

        return ar;
    }
}
