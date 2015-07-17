/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <vector>

#include "Common.h"
#include "VectorContexts.h"
#include "VectorEnums.h"
#include "Configure.h"

namespace Kernel
{
    class VectorSpeciesParameters : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        static std::map< std::string, const VectorSpeciesParameters* > _vspMap;
        static VectorSpeciesParameters* CreateVectorSpeciesParameters(const std::string& vector_species_name);
        virtual ~VectorSpeciesParameters();
        bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        std::vector<float> habitat_param;
        std::vector<VectorHabitatType::Enum> habitat_type;
        float aquaticarrhenius1;
        float aquaticarrhenius2;
        float infectedarrhenius1;
        float infectedarrhenius2;
        float immatureduration;
        float daysbetweenfeeds;
        float anthropophily;
        float eggbatchsize;
        float infectedeggbatchmod;
        float infectiousmortalitymod;
        float aquaticmortalityrate;
        float adultlifeexpectancy;
        float transmissionmod;
        float acquiremod;
        float infectioushfmortmod;
        float indoor_feeding;

        // derived values (e.g. 1/daysbetweenfeeds = feedingrate)
        float feedingrate;
        float adultmortality;
        float immaturerate;

    protected:
        VectorSpeciesParameters();
        void Initialize(const std::string& vector_species_name);

    private:
        std::string _species;

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template< class Archive >
        friend void serialize(Archive & ar, VectorSpeciesParameters& pars, const unsigned int  file_version );
#endif
    };
}
