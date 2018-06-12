/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "OutbreakIndividual.h"

namespace Kernel
{
    class OutbreakIndividualMalaria : public OutbreakIndividual
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED( InterventionFactory, OutbreakIndividualMalaria, IDistributableIntervention )

    public:
        OutbreakIndividualMalaria();
        virtual ~OutbreakIndividualMalaria();

        // OutbreakIndividual methods
        virtual bool Configure( const Configuration * inputJson ) override;
        virtual void ConfigureGenome( const Configuration * inputJson );
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject );

    protected:
        std::vector<std::string> m_GenomeMarkerNames;
        bool m_CreateRandomGenome;
    };
}
