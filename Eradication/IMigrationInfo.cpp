/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
#ifdef ENABLE_DENGUE
                || (sim_type == SimType::DENGUE_SIM)
#endif
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
