
#include "stdafx.h"
#include "IMigrationInfo.h"
#include "Migration.h"
#ifndef DISABLE_VECTOR
#include "MigrationInfoVector.h"
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
            IMigrationInfoFactory* p_mif = nullptr ;
#ifndef DISABLE_VECTOR
            if( (sim_type == SimType::VECTOR_SIM) || (sim_type == SimType::MALARIA_SIM)
                )
            {
                if( useDefaultMigration )
                {
                    p_mif = new MigrationInfoFactoryVectorDefault( defaultTorusSize );
                }
                else
                {
                    p_mif = new MigrationInfoFactoryVector();
                }
            }
            else 
#endif
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
    }
}
