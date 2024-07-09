
#include "stdafx.h"
#include "StrainIdentity.h"
#include "Infection.h"
#include "SimulationConfig.h"
#include "Log.h"

SETUP_LOGGING( "StrainIdentity" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( StrainIdentity )
    END_QUERY_INTERFACE_BODY( StrainIdentity )

    StrainIdentity::StrainIdentity(void)
        : antigenID(0)
        , geneticID(0)
    {
    }

    StrainIdentity::StrainIdentity(int initial_antigen, int initial_genome, RANDOMBASE * pRng )
        : antigenID( initial_antigen )
        , geneticID( initial_genome )
    {
        if( initial_antigen >= int(InfectionConfig::number_basestrains) )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "initial_genome", initial_antigen, InfectionConfig::number_basestrains );
        }

        if ( initial_genome < 0 )
        {
            int substrains = InfectionConfig::number_substrains;
            if (substrains & (substrains-1))
            {
                std::stringstream ss;
                ss << "'Number_Substrains'=" << substrains << "  Only supporting random genome generation for Number_Substrains as factor of two.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            unsigned int BARCODE_BITS = 0;
            while(substrains >>= 1) ++BARCODE_BITS;
            geneticID = pRng->ul() & ((1 << BARCODE_BITS)-1);
            //genome = pIndiv->GetRng()->uniformZeroToN16(simConfigObj->number_substrains);
            LOG_DEBUG_F("random genome generation... antigen: %d\t genome: %d\n", antigenID, geneticID);
        }
        else if( initial_genome >= int(InfectionConfig::number_substrains) )
        {
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "initial_genome", initial_genome, InfectionConfig::number_substrains );
        }
    }

    StrainIdentity::StrainIdentity( const StrainIdentity& rMaster )
        : antigenID( rMaster.GetAntigenID() )
        , geneticID( rMaster.GetGeneticID() )
    {
        LOG_DEBUG_F( "New infection with antigen id %d and genetic id %d\n", antigenID, geneticID );
    }

    StrainIdentity::~StrainIdentity(void)
    {
    }

    IStrainIdentity* StrainIdentity::Clone() const
    {
        IStrainIdentity* p_clone = new StrainIdentity( *this );
        return p_clone;
    }

    int StrainIdentity::GetAntigenID(void) const
    {
        return antigenID;
    }

    int StrainIdentity::GetGeneticID(void) const
    {
        return geneticID;
    }

    void StrainIdentity::SetAntigenID(int in_antigenID)
    {
        antigenID = in_antigenID;
    }

    void StrainIdentity::SetGeneticID(int in_geneticID)
    {
        geneticID = in_geneticID;
    }

    REGISTER_SERIALIZABLE(StrainIdentity);

    void StrainIdentity::serialize(IArchive& ar, StrainIdentity* obj)
    {
        StrainIdentity& strain = *obj;
        ar.labelElement("antigenID") & strain.antigenID;
        ar.labelElement("geneticID") & strain.geneticID;
    }

    IArchive& StrainIdentity::serialize(IArchive& ar, StrainIdentity& strain)
    {
        ar.startObject();
            ar.labelElement("antigenID") & strain.antigenID;
            ar.labelElement("geneticID") & strain.geneticID;
        ar.endObject();

        return ar;
    }
}
