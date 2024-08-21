
#include "stdafx.h"
#include "IMigrationInfo.h"
#include "Migration.h"
#ifndef DISABLE_VECTOR
#include "MigrationInfoVector.h"
#include "VectorParameters.h"
#include "SimulationConfig.h"
#endif

SETUP_LOGGING( "IMigrationInfo" )

namespace Kernel
{
    namespace MigrationFactory
    {
        IMigrationInfoFactory* ConstructMigrationInfoFactory( const ::Configuration *config, 
                                                              const std::string& idreference,
                                                              SimType::Enum sim_type,
                                                              MigrationStructure::Enum ms,
                                                              bool useDefaultMigration,
                                                              int defaultTorusSize )
        {
            IMigrationInfoFactory* p_mif = nullptr;
            if( useDefaultMigration )
            {
                p_mif = new MigrationInfoFactoryDefault( defaultTorusSize );
            }
            else
            {
                p_mif = new MigrationInfoFactoryFile();
            }
            p_mif->Initialize( config, idreference );
            return p_mif;
        }

#ifndef DISABLE_VECTOR
        IMigrationInfoFactoryVector* ConstructMigrationInfoFactoryVector( JsonConfigurable* pParent, const ::Configuration* config )
        {
            // -------------------------------------------------------------------------
            // --- We have to get these via the global because we are now creating this
            // --- when we are creating VectorSpeciesParameters
            // -------------------------------------------------------------------------
            bool use_defalt_demog        = false;
            int default_torus_size       = 9;
            bool enable_vector_migration = true; // true so we create the factor with the schema parameters

            if( GET_CONFIGURABLE( SimulationConfig ) != nullptr )
            {
                use_defalt_demog        = !GET_CONFIGURABLE( SimulationConfig )->demographics_initial;
                default_torus_size      = GET_CONFIGURABLE( SimulationConfig )->default_torus_size;
            }
            IMigrationInfoFactoryVector* p_mifv = nullptr ;

            if( use_defalt_demog )
            {
                p_mifv = new MigrationInfoFactoryVectorDefault( enable_vector_migration, default_torus_size );
            }
            else
            {
                p_mifv = new MigrationInfoFactoryVector( enable_vector_migration );
            }
            p_mifv->ReadConfiguration( pParent, config );
            return p_mifv;
        }
#endif
    }

}
