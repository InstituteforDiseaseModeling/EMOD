/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "VectorTransmissionGroups.h"
#include "Exceptions.h"
#include "VectorPopulation.h"
#include "IInfectable.h"
#include "IContagionPopulation.h"


// These includes are required to bring in randgen
#include "Environment.h"
#include "Contexts.h"
#include "RANDOM.h"

SETUP_LOGGING( "VectorNodeTransmissionGroups" )


namespace Kernel
{
    VectorTransmissionGroups::VectorTransmissionGroups()
    {
    }

    void VectorTransmissionGroups::ExposeToContagion( IInfectable* candidate, const TransmissionGroupMembership_t* transmissionGroupMembership, float deltaTee ) const
    {
#ifndef DISABLE_VECTOR
        for (int iAntigen = 0; iAntigen < antigenCount; iAntigen++)
        {
            const vector<TransmissionGroupsBase::ContagionAccumulator_t>& forceOfInfectionForRouteGroup = forceOfInfectionByAntigenRouteGroup[iAntigen];
            int routeIndex;
            int groupindex;
            for (const auto& entry : (*transmissionGroupMembership))
            {
                routeIndex = entry.first;
                groupindex = entry.second;
                float forceOfInfection = forceOfInfectionForRouteGroup[routeIndex][groupindex];
                vector<const SubstrainMap_t*> substrainDistributions(1, &sumInfectivityByAntigenRouteGroupSubstrain[iAntigen][routeIndex][groupindex]);
                if ((forceOfInfection > 0) && (candidate != nullptr))
                {
                    SubstrainPopulationImpl contagionPopulation(iAntigen, forceOfInfection, substrainDistributions);
                     // Are you a human or mosquito?
                    IVectorPopulation *ivp;
                    IIndividualHumanEventContext *ihec;
                    TransmissionRoute::Enum route;
                    if (s_OK == candidate->QueryInterface(GET_IID(IVectorPopulation), (void**)&ivp) )
                    {
                        route = (routeNames[routeIndex] == string("Indoor")) ? TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR : TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR;
                    }
                    else if (s_OK == candidate->QueryInterface(GET_IID(IIndividualHumanEventContext), (void**)&ihec) )
                    {
                        route = (routeNames[routeIndex] == string("Indoor")) ? TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR : TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR;
                    }
                    else
                    {
                        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ihec", "<multiple>", "IIndividualHumanEventContext" );
                    }
                    LOG_DEBUG_F("ExposeToContagion: [Antigen:%d] Route:%d, Group:%d, exposure = %f\n", iAntigen, routeIndex, groupindex, forceOfInfection );
                    candidate->Expose((IContagionPopulation*)&contagionPopulation, deltaTee, route);
                }
            }
        }
#endif
    }
}
