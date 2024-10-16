
#include "stdafx.h"
#include "ReportFactory.h"
#include "ObjectFactoryTemplates.h"
#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "ReportFactory" )

namespace Kernel
{
    ReportFactory* ReportFactory::_instance = nullptr;

    template ReportFactory* ObjectFactory<IReport, ReportFactory>::getInstance();


    ReportFactory::ReportFactory()
        : ObjectFactory<IReport, ReportFactory>()
        , m_ArrayElementName( "Reports" )
        , m_UseDefaults( false )
    {
        if( _instance != nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__,"should be null");
        }
    }

    IReport* ReportFactory::CreateInstance( const Configuration *config,
                                            const char* parameterName,
                                            bool nullOrEmptyOrNoClassNotError )
    {
        // --------------------------------------------------------------------------------------
        // --- Create object and configure
        // ---
        // --- Keeping this simple. But bear in mind CreateInstanceFromSpecs can throw exception
        // --- and JC::_useDefaults will not be restored. But we won't keep running in that case.
        // --------------------------------------------------------------------------------------
        bool reset = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = m_UseDefaults;

        IReport* p_report = ObjectFactory<IReport, ReportFactory>::CreateInstance( config, parameterName, nullOrEmptyOrNoClassNotError );

        JsonConfigurable::_useDefaults = reset;

        // ------------------------------------------------------------------------
        // --- Verify that the object is supported for the current simulation type
        // ------------------------------------------------------------------------
        CheckSimType( p_report );

        return p_report;
    }

    std::vector<IReport*> ReportFactory::Load( const std::string& rFilename )
    {
        // ------------------
        // --- Load JSON file
        // ------------------
        Configuration *p_file_config = Configuration::Load( rFilename );

        // ---------------------------------------------
        // --- Read/Set flag about using default values
        // ---------------------------------------------
        if( p_file_config->Exist( "Use_Defaults" ) )
        {
            // store value of Use_Defaults from file in the factory.
            m_UseDefaults = ((*p_file_config)[ "Use_Defaults" ].As< json::Number >() != 0);
        }
        else
        {
            LOG_INFO_F( "Do NOT Use default values for %s when key not found since not specified in the file and 'default' is false.\n",rFilename.c_str() );
            m_UseDefaults = false;
        }

        // ------------------------------------------------------------
        // --- Get array of objects and check for errors getting object
        // ------------------------------------------------------------
        Array object_array;
        try
        {
            object_array = (*p_file_config)[ m_ArrayElementName ].As<Array>();
        }
        catch( json::Exception &e )
        {
            std::stringstream ss;
            ss << "Error reading the element '" << m_ArrayElementName << "' in " << rFilename << ".\n";
            ss << "Error msg= " << e.what();
            throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // -----------------------------------------------------
        // --- Instantiate an object for each entry in the file
        // -----------------------------------------------------
        std::vector<IReport*> object_list;
        for( int k = 0; k < object_array.Size(); k++ )
        {
            std::stringstream param_name;
            param_name << "Reports[" << k << "]";
            IReport* p_report = ObjectFactory<IReport, ReportFactory>::CreateInstance( object_array[ k ],
                                                                                       p_file_config->GetDataLocation(),
                                                                                       param_name.str().c_str() );
            object_list.push_back( p_report );
        }

        return object_list;
    }
}
