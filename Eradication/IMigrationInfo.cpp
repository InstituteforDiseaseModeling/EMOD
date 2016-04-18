/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "IMigrationInfo.h"
#include "Migration.h"
#ifndef DISABLE_VECTOR
#include "MigrationInfoVector.h"
#endif

static const char * _module = "IMigrationInfo";

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
            bool enable_human_migration = (ms != MigrationStructure::NO_MIGRATION);

            IMigrationInfoFactory* p_mif = nullptr ;
#ifndef DISABLE_VECTOR
            if( (sim_type == SimType::VECTOR_SIM) || (sim_type == SimType::MALARIA_SIM) )
            {
                if( useDefaultMigration )
                {
                    p_mif = new MigrationInfoFactoryVectorDefault( enable_human_migration, defaultTorusSize );
                }
                else
                {
                    p_mif = new MigrationInfoFactoryVector( enable_human_migration );
                }
            }
            else 
#endif
            if( useDefaultMigration )
            {
                p_mif = new MigrationInfoFactoryDefault( enable_human_migration, defaultTorusSize );
            }
            else
            {
                p_mif = new MigrationInfoFactoryFile( enable_human_migration );
            }
            p_mif->Initialize( config, idreference );
            return p_mif;
        }
    }
}
