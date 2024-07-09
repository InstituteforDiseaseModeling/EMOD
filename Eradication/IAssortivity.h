
#pragma once

#include <list>
#include "IdmApi.h"
#include "IdmDateTime.h"
#include "Configure.h"
#include "Configuration.h"
#include "IIndividualHumanSTI.h"
#include "ISerializable.h"

namespace Kernel 
{
    class IDMAPI IAssortivity : public virtual ISerializable, public JsonConfigurable
    {
    public:
        virtual ~IAssortivity() {};

        virtual bool Configure(const Configuration *config) = 0;

        // Update assortivity parameters/controls.  For example, one might want the
        // assortivity to change based on the year.
        virtual void Update( const IdmDateTime& rCurrentTime, float dt ) = 0;

        // Using the attributes of pPartnerA and the attributes of the potential partners, 
        // select a partner from the potentialPartnerList.
        // Return nullptr if a suitable partner was not found.
        // nullptr can be returned even if the list is not empty.
        virtual IIndividualHumanSTI* SelectPartner( IIndividualHumanSTI* pPartnerA,
                                                    const list<IIndividualHumanSTI*>& potentialPartnerList ) = 0;

        // used when deserializing an object to get it configured properly
        virtual void SetParameters( RANDOMBASE* prng ) = 0;
    };
}