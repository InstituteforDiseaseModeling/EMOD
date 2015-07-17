/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "CajunIncludes.h"

namespace Kernel
{
    struct IDistributableIntervention ;
    struct INodeDistributableIntervention;

    // This interface is used with the InterventionValidator to validate the intervention
    // for disease specific features.  There maybe values that are allowable for the 
    // intervention class but not allowable given specific settings for a given disease.
    // For example, the disease may have a set of strings that are allowable values for
    // a string field in an intervention type.  This gives a way to make sure that the
    // intervention has one of the allowable values.
    struct IDiseaseSpecificValidator
    {
        virtual void Validate( const std::string& rClassName,
                               IDistributableIntervention* pInterventionToValidate ) = 0 ;

        virtual void Validate( const std::string& rClassName,
                               INodeDistributableIntervention* pInterventionToValidate ) = 0 ;
    };

    // Determine if an intervention is valid and throw an exception if not.
    class InterventionValidator
    {
    public:
        // These functions verify that the intervention defined in the given rElement
        // is valid for the current simulation type.  The goal is to determine during
        // initialization that the interventions being used are appropriate for the
        // current simulation type.
        static void ValidateInterventionArray( const json::Element& rElement );
        static void ValidateIntervention( const json::Element& rElement );

        static void SetDiseaseSpecificValidator( IDiseaseSpecificValidator* pValidator );
    private:
        static IDiseaseSpecificValidator* m_pDiseaseSpecificValidator ;
    };
}