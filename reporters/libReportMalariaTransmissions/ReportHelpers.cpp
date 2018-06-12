/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "StdAfx.h"
#include "ReportHelpers.h"

#include "Log.h"
#include "MalariaContexts.h"
#include "IIndividualHuman.h"
#include "IndividualEventContext.h"
#include "ReportUtilities.h"
#include "Serializer.h"

SETUP_LOGGING("MalariaTransmissionReport")

namespace Kernel
{
    MalariaInfectionInfo::MalariaInfectionInfo(IInfection* infection)
    {
        id = infection->GetSuid().data;

        IInfectionMalaria* infection_malaria = NULL;
        if (s_OK != infection->QueryInterface(GET_IID(IInfectionMalaria), (void**)&infection_malaria) )
        {
            throw QueryInterfaceException(
                __FILE__, __LINE__, __FUNCTION__,
                "infection", "IInfectionMalaria", "IInfection");
        }

        asexual_density = infection_malaria->get_asexual_density();
        gametocyte_density = infection_malaria->get_mature_gametocyte_density();
    }


    MalariaIndividualInfo::MalariaIndividualInfo(IIndividualHuman* individual)
    {
        SetProperties(individual);
    }

    MalariaIndividualInfo::MalariaIndividualInfo(IIndividualHumanEventContext* context)
    {
        IIndividualHuman * individual = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHuman), (void**)&individual) )
        {
            throw QueryInterfaceException(
                __FILE__, __LINE__, __FUNCTION__,
                "context", "IIndividualHuman", "IIndividualHumanEventContext");
        }

        SetProperties(individual);
    }

    MalariaIndividualInfo::~MalariaIndividualInfo()
    {
        for (auto inf: infections)
        {
            delete inf;
        }
    }

    void MalariaIndividualInfo::SetProperties(IIndividualHuman* individual)
    {
        id = individual->GetSuid().data;
        age = individual->GetAge();
        infectiousness = individual->GetInfectiousness();

        const infection_list_t& infection_list = individual->GetInfections();
        for (auto inf: infection_list)
        {
            infections.push_back(_new_ MalariaInfectionInfo(inf));
        }
    }


    Transmission::Transmission(Location location_,
        uint32_t txId_, std::vector<uint32_t> txInfIds_, std::vector<float> txGams_,
        uint32_t acId_, uint32_t acInfId_)
    : location(location_)
    , transmitIndividualId(txId_)
    , transmitInfectionIds(txInfIds_)
    , transmitGametocyteDensities(txGams_)
    , acquireIndividualId(acId_)
    , acquireInfectionId(acInfId_)
    {}

    void Transmission::Serialize(IJsonObjectAdapter& root, JSerializer& helper)
    {
        //LOG_DEBUG("Serializing Transmission\n");
        root.BeginObject();

        //LOG_DEBUG("Inserting transmission variables\n");
        root.Insert("timestep", location.second);
        root.Insert("node_id", location.first);
        root.Insert("acquireIndividualId", acquireIndividualId);
        root.Insert("acquireInfectionId", acquireInfectionId);
        root.Insert("transmitIndividualId", transmitIndividualId);

        root.Insert("transmitInfectionIds");
        helper.JSerialize(transmitInfectionIds, &root);

        root.Insert("transmitGametocyteDensities");
        helper.JSerialize(transmitGametocyteDensities, &root);

        root.EndObject();
    }


    ClinicalSample::ClinicalSample(Location location_, std::string event_,
        uint32_t id_, std::vector<uint32_t> infIds_, std::vector<float> densities_)
    : location(location_)
    , sample_event(event_)
    , individualId(id_)
    , infectionIds(infIds_)
    , parasiteDensities(densities_)
    {}

    void ClinicalSample::Serialize(IJsonObjectAdapter& root, JSerializer& helper)
    {
        //LOG_DEBUG("Serializing ClinicalSample\n");
        root.BeginObject();

        //LOG_DEBUG("Inserting clinical-sample variables\n");
        root.Insert("timestep", location.second);
        root.Insert("node_id", location.first);
        root.Insert("individualId", individualId);
        root.Insert("sample_event", sample_event.c_str());

        root.Insert("infectionIds");
        helper.JSerialize(infectionIds, &root);

        root.Insert("parasiteDensities");
        helper.JSerialize(parasiteDensities, &root);

        root.EndObject();
    }
}