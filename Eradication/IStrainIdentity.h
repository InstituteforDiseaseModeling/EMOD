
#pragma once

#include "ISerializable.h"

namespace Kernel
{
    struct IStrainIdentity : ISerializable
    {
        virtual IStrainIdentity* Clone() const = 0;
        virtual int  GetAntigenID(void) const = 0;
        virtual int  GetGeneticID(void) const = 0;
        virtual void SetAntigenID(int in_antigenID) = 0;
        virtual void SetGeneticID(int in_geneticID) = 0;

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
