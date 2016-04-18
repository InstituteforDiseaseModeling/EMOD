/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Diagnostics.h"

namespace Kernel
{
    class SmearDiagnostic : public SimpleDiagnostic 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SmearDiagnostic, IDistributableIntervention)

    public: 
        SmearDiagnostic();
        SmearDiagnostic( const SmearDiagnostic& );
        virtual bool Configure( const Configuration* pConfig );
        virtual ~SmearDiagnostic();// { }
        virtual bool positiveTestResult();

        DECLARE_SERIALIZABLE(SmearDiagnostic);
    };
}
