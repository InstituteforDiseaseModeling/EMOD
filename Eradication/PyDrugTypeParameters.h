/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#if 0
#include "Configure.h"
#include "InterventionEnums.h"
#include "PyDrugTypeParameters.h"

namespace Kernel {
    class PyDrugTypeParameters : public JsonConfigurable
    {
        friend class SimulationConfig;
        friend class AntipoliovirusDrug;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        PyDrugTypeParameters( const PyDrugType::Enum& drugType );
        static PyDrugTypeParameters* CreatePyDrugTypeParameters( const PyDrugType::Enum& drugType );
        virtual ~PyDrugTypeParameters();
        bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        static map< int, PyDrugTypeParameters* > _pdtMap;


    protected:

    private:
        PyDrugType::Enum _drugType;
    };
}
#endif