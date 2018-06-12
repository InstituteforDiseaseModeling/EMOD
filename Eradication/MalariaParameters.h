/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include <map>

#include "Malaria.h"

namespace Kernel
{
    struct IGenomeMarkers;
    class MalariaDrugTypeParameters;

    struct MalariaParameters
    {
        PKPDModel::Enum PKPD_model; // PKPD_MODEL

        // suscept malaria (TODO: these can probably move to SusceptibilityMalariaConfig as well, as long as the parasiteSmearSensitivity usage in IndividualHumanMalaria can be addressed)
        float parasiteSmearSensitivity;
        float newDiagnosticSensitivity;

        // antigen population
        int falciparumMSPVars;
        int falciparumNonSpecTypes;
        int falciparumPfEMP1Vars;

        float feverDetectionThreshold;

        std::vector<std::string> genome_marker_names; // temporary variable used to initialize pGenomeMarkers
        IGenomeMarkers* pGenomeMarkers;

        std::map< std::string, MalariaDrugTypeParameters * > MalariaDrugMap;


        MalariaParameters()
        : PKPD_model(PKPDModel::FIXED_DURATION_CONSTANT_EFFECT)
        , parasiteSmearSensitivity(-42.0f)
        , newDiagnosticSensitivity(-42.0f)
        , falciparumMSPVars(0)
        , falciparumNonSpecTypes(0)
        , falciparumPfEMP1Vars(0)
        , feverDetectionThreshold(-42.0f)
        , genome_marker_names()
        , pGenomeMarkers(nullptr)
        , MalariaDrugMap()
        {
        }
    };
}
