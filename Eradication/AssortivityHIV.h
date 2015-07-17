/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <vector>
#include "Assortivity.h"

namespace Kernel 
{
    // AssortivityHIV adds the groups of STI CoInfection Status and HIV Infection Status
    class IDMAPI AssortivityHIV : public Assortivity
    {
    public:
        AssortivityHIV( RelationshipType::Enum relType, RANDOMBASE *prng );
        virtual ~AssortivityHIV();

        // -------------------------
        // --- Assortivity Methods
        // -------------------------
        // Update assortivity parameters/controls.  For example, one might want the
        // assortivity to change based on the year.
        virtual void Update( const IdmDateTime& rCurrentTime, float dt );

    protected:
        virtual IIndividualHumanSTI* SelectPartnerForExtendedGroups( AssortivityGroup::Enum group,
                                                                     const IIndividualHumanSTI* pPartnerA,
                                                                     const list<IIndividualHumanSTI*>& potentialPartnerList );

        // This routine is called inside Configure() but before the data is completely read.
        virtual void AddConfigurationParameters( AssortivityGroup::Enum group, const Configuration *config );

        virtual void CheckDerivedValues() ;
        virtual void CheckAxesForReceivedResults();

        virtual AssortivityGroup::Enum GetGroupToUse() const ;
    private:
        float m_StartYear ;  // if current year is < start year, default to NO_GROUP
        bool  m_StartUsing ; // value is based on start year versus current year
    };
}