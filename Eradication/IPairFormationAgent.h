
#pragma once

#include "IdmApi.h"
#include "IdmDateTime.h"
#include "IIndividualHumanSTI.h"
#include "Configure.h"
#include "ISerializable.h"
#include <map>
#include <vector>

using namespace std;

namespace Kernel
{
    class IPKeyValue;
    struct Sigmoid;

    struct IDMAPI IPairFormationAgent : virtual ISerializable, JsonConfigurable
    {
        virtual void BeginUpdate() = 0;
        virtual void AddIndividual(IIndividualHumanSTI*) = 0;
        virtual void RemoveIndividual(IIndividualHumanSTI*) = 0;
        virtual void Update( const IdmDateTime& rCurrentTime, float dt ) = 0;
        virtual void RegisterIndividual( IIndividualHumanSTI* pIndividualSti ) = 0;
        virtual void UnregisterIndividual( IIndividualHumanSTI* pIndividualSti ) = 0;
        virtual bool StartNonPfaRelationship( IIndividualHumanSTI* pIndividualSti,
                                              const IPKeyValue& rPartnerHasIP,
                                              Sigmoid* pCondomUsage ) = 0;
        virtual const map<int, vector<float>>& GetAgeBins() = 0;
        virtual const map<int, vector<float>>& GetDesiredFlow() = 0;
        virtual const map<int, vector<int>>& GetQueueLengthsBefore() = 0;
        virtual const map<int, vector<int>>& GetQueueLengthsAfter() = 0;
        virtual void Print(const char *) const = 0;
        virtual ~IPairFormationAgent() {}
    };
}