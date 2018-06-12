/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"

using namespace Kernel;

struct ICampaignCostObserverFake : public ICampaignCostObserver
{
    virtual ~ICampaignCostObserverFake() {};

    virtual void notifyCampaignExpenseIncurred( float expenseIncurred, const IIndividualHumanEventContext * pIndiv ) {};
    virtual void notifyCampaignEventOccurred( ISupports * pDistributedIntervention,
                                              ISupports * pDistributor, 
                                              IIndividualHumanContext * pDistributeeIndividual ) {};

    virtual QueryResult QueryInterface(iid_t iid, void** pinstance) { return QueryResult::e_NOINTERFACE ; };
    virtual int32_t AddRef()  { return -1 ; };
    virtual int32_t Release() { return -1 ; };
};
