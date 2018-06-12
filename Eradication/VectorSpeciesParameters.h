/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>

#include "Common.h"
#include "VectorContexts.h"
#include "VectorEnums.h"
#include "Configure.h"

namespace Kernel
{
    class LarvalHabitatParams : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            LarvalHabitatParams() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }
            std::map< VectorHabitatType::Enum, const Configuration* > habitat_map;
    };

    class IDMAPI VectorSpeciesParameters : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        static VectorSpeciesParameters* CreateVectorSpeciesParameters( const Configuration* inputJson, 
                                                                       const std::string& vector_species_name );
        virtual ~VectorSpeciesParameters();
        bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        LarvalHabitatParams habitat_params;
        float aquaticarrhenius1;
        float aquaticarrhenius2;
        float infectedarrhenius1;
        float infectedarrhenius2;
        float cyclearrhenius1;
        float cyclearrhenius2;
        float cyclearrheniusreductionfactor;
        float immatureduration;
        float daysbetweenfeeds;
        float anthropophily;
        float eggbatchsize;
        float infectedeggbatchmod;
        float eggsurvivalrate;
        float infectiousmortalitymod;
        float aquaticmortalityrate;
        float adultlifeexpectancy;
        float transmissionmod;
        float acquiremod;
        float infectioushfmortmod;
        float indoor_feeding;
        float nighttime_feeding;

        // derived values (e.g. 1/adultlifeexpectanc = adultmortality)
        float adultmortality;
        float immaturerate;

        static void serialize(IArchive&, VectorSpeciesParameters*&);

    protected:
        VectorSpeciesParameters();
        void Initialize(const std::string& vector_species_name);

    private:
        std::string _species;

#pragma warning( pop )
    };
}
