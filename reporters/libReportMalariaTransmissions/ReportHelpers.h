/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <map>
#include <memory>
#include <tuple>

#include "INodeContext.h"


namespace Kernel
{
    struct IInfection;
    class IInfectionMalaria;
    struct MalariaInfectionInfo
    {
        uint32_t id;
        float asexual_density;
        float gametocyte_density;

        MalariaInfectionInfo(IInfection* infection);
    };


    struct IIndividualHuman;
    struct IIndividualHumanEventContext;
    struct MalariaIndividualInfo
    {
        uint32_t id;
        float age;
        float infectiousness;
        std::vector<MalariaInfectionInfo*> infections;

        MalariaIndividualInfo(IIndividualHuman* individual);
        MalariaIndividualInfo(IIndividualHumanEventContext* context);
        virtual ~MalariaIndividualInfo();

    protected:
        void SetProperties(IIndividualHuman* individual);
    };


    typedef std::vector<std::unique_ptr<MalariaIndividualInfo>> InfectiousReservoir_t;

    typedef uint64_t InfectedMosquito_t;
    typedef std::vector<InfectedMosquito_t> InfectiousMosquitos_t;

    typedef std::pair<ExternalNodeId_t, int> Location;  // (node external id, time step)


    struct IJsonObjectAdapter;
    class JSerializer;
    struct Transmission
    {
        Location location;

        uint32_t transmitIndividualId;  // infection-transmitting individual
        std::vector<uint32_t> transmitInfectionIds;
        std::vector<float> transmitGametocyteDensities;

        uint32_t acquireIndividualId;  // infection-acquiring individual
        uint32_t acquireInfectionId;

        Transmission(Location location_,
            uint32_t txId_, std::vector<uint32_t> txInfIds_, std::vector<float> txGams_,
            uint32_t acId_, uint32_t acInfId_);

        void Serialize(IJsonObjectAdapter& root, JSerializer& helper);
    };

    struct ClinicalSample
    {
        Location location;
        std::string sample_event;

        uint32_t individualId;
        std::vector<uint32_t> infectionIds;
        std::vector<float> parasiteDensities;

        ClinicalSample(Location location_, std::string event_,
            uint32_t id_, std::vector<uint32_t> infIds_, std::vector<float> densities_);

        void Serialize(IJsonObjectAdapter& root, JSerializer& helper);
    };
}