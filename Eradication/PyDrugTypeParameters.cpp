/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "PyDrugTypeParameters.h"

#include "Exceptions.h"
#include <algorithm> // for transform
#include <cctype> // for tolower

static const char * _module = "PyDrugTypeParameters";

std::map< int, Kernel::PyDrugTypeParameters* > Kernel::PyDrugTypeParameters::_pdtMap;

namespace Kernel
{
    PyDrugTypeParameters::PyDrugTypeParameters(
        const PyDrugType::Enum &drugType
    )
    { LOG_DEBUG( "ctor\n" );
    }

    PyDrugTypeParameters::~PyDrugTypeParameters()
    { LOG_DEBUG( "dtor\n" );
    }

    PyDrugTypeParameters* PyDrugTypeParameters::CreatePyDrugTypeParameters(
        const PyDrugType::Enum &drugType
    )
    { 
        LOG_DEBUG( "Create\n" );

        PyDrugTypeParameters* params = NULL;
        map< int, PyDrugTypeParameters* >::const_iterator itMap = _pdtMap.find(drugType);

        if ( itMap == _pdtMap.end() )
        {
            params = _new_ PyDrugTypeParameters( drugType );
            _pdtMap[ drugType ] = params;
            return params;
        }
        else
        {
            return itMap->second;
        }
    }

    bool
    PyDrugTypeParameters::Configure(
        const ::Configuration *config
    )
    {
        LOG_DEBUG( "Configure\n" );

        return JsonConfigurable::Configure( config );
    }

    QueryResult
    PyDrugTypeParameters::QueryInterface(
        iid_t iid, void **ppvObject
    )
    {
        throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__ );
    }
}

