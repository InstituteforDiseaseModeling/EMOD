/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>
#include <vector>

#include "BoostLibWrapper.h"
#include "Configure.h"
#include "Properties.h"

namespace Kernel
{
    struct IIndividualHumanEventContext;

    class IDMAPI PropertyRestrictions : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
    public:
        PropertyRestrictions();
        virtual void ConfigureFromJsonAndKey( const Configuration *, const std::string &key ) override;
        virtual json::QuickBuilder GetSchema() override;

        int Size() const;
        void Add( std::map< std::string, std::string >& rMap );
        bool Qualifies( const IIndividualHumanEventContext* pHEC );
        std::string GetAsString() const;

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::list< IPKeyValueContainer > _restrictions;
#pragma warning( pop )
    };
}
