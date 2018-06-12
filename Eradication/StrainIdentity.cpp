/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "StrainIdentity.h"
#include "Log.h"

SETUP_LOGGING( "StrainIdentity" )

namespace Kernel {

    StrainIdentity::StrainIdentity(void)
    {
        antigenID = 0;
        geneticID = 0;
    }

    StrainIdentity::StrainIdentity(int initial_antigen, int initial_genome)
    {
        antigenID = initial_antigen;
        geneticID = initial_genome;
    }

    StrainIdentity::StrainIdentity( const IStrainIdentity *copy )
    {
        antigenID = copy->GetAntigenID();
        geneticID = copy->GetGeneticID();
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
