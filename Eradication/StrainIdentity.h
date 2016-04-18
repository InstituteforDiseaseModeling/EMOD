/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"
#include "IArchive.h"

namespace Kernel
{
    class StrainIdentity
    {
    public:
        StrainIdentity(void);
        StrainIdentity(int initial_antigen, int initial_genome);
        virtual ~StrainIdentity(void);

        int  GetAntigenID(void) const;
        int  GetGeneticID(void) const;
        void SetAntigenID(int in_antigenID);
        void SetGeneticID(int in_geneticID);

        // Order first by antigenID, then by geneticID
        inline bool operator<(const StrainIdentity& id) const {
            return ( antigenID <  id.GetAntigenID() ) || 
                   ( antigenID == id.GetAntigenID() && geneticID < id.GetGeneticID() );
        }

        inline bool operator>(const StrainIdentity& id) const {
            return ( antigenID >  id.GetAntigenID() ) || 
                   ( antigenID == id.GetAntigenID() && geneticID > id.GetGeneticID() );
        }

        friend IArchive& serialize(IArchive&, StrainIdentity*&);

    protected:
        int antigenID;
        int geneticID;
    };
}
