/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

namespace Kernel
{
    struct IStrainIdentity
    {
        virtual int  GetAntigenID(void) const = 0;
        virtual int  GetGeneticID(void) const = 0;
        virtual void SetAntigenID(int in_antigenID) = 0;
        virtual void SetGeneticID(int in_geneticID) = 0;
        virtual void ResolveInfectingStrain( IStrainIdentity* strainId ) const = 0;

        // Order first by antigenID, then by geneticID
        inline virtual bool operator<(const IStrainIdentity& id) const 
        {
            return ( this->GetAntigenID() <  id.GetAntigenID() ) || 
                   ( this->GetAntigenID() == id.GetAntigenID() && this->GetGeneticID() < id.GetGeneticID() );
        }

        inline virtual bool operator>(const IStrainIdentity& id) const 
        {
            return ( this->GetAntigenID() >  id.GetAntigenID() ) ||
                   ( this->GetAntigenID() == id.GetAntigenID() && this->GetGeneticID() > id.GetGeneticID() );
        }
    };
}
