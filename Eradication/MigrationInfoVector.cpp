
#include "stdafx.h"
#include "MigrationInfoVector.h"
#include "INodeContext.h"
#include "VectorContexts.h"


namespace Kernel
{
#define ENABLE_VECTOR_MIGRATION_NAME "Enable_Vector_Migration"
#define MODIFIER_EQUATION_NAME "Vector_Migration_Modifier_Equation"

    // ------------------------------------------------------------------------
    // --- MigrationInfoVector
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( MigrationInfoNullVector, MigrationInfoNull )
    END_QUERY_INTERFACE_DERIVED( MigrationInfoNullVector, MigrationInfoNull )

    MigrationInfoNullVector::MigrationInfoNullVector()
    : MigrationInfoNull()
    {
    }

    MigrationInfoNullVector::~MigrationInfoNullVector()
    {
    }


    // ------------------------------------------------------------------------
    // --- MigrationInfoVector
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(MigrationInfoVector, MigrationInfoFixedRate)
    END_QUERY_INTERFACE_DERIVED(MigrationInfoVector, MigrationInfoFixedRate)

    MigrationInfoVector::MigrationInfoVector( INodeContext * _parent,
                                              ModiferEquationType::Enum equation,
                                              float habitatModifier,
                                              float foodModifier,
                                              float stayPutModifier ) 
    : MigrationInfoFixedRate( _parent, false ) 
    , m_RawMigrationRate()
    , m_ThisNodeId(suids::nil_suid())
    , m_ModifierEquation(equation)
    , m_ModifierHabitat(habitatModifier)
    , m_ModifierFood(foodModifier)
    , m_ModifierStayPut(stayPutModifier)
    {
    }

    MigrationInfoVector::~MigrationInfoVector() 
    {
    }

    void MigrationInfoVector::Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData )
    {
        MigrationInfoFixedRate::Initialize( rRateData );
    }

    void MigrationInfoVector::SaveRawRates( std::vector<float>& r_rate_cdf )
    {
        // ---------------------------------------------------------
        // --- Keep the un-normalized rates so we can multiply them
        // --- times our food adjusted rates.
        // ---------------------------------------------------------
        m_RawMigrationRate.clear();
        for( int i = 0; i < r_rate_cdf.size(); i++)
        {
            m_RawMigrationRate.push_back( r_rate_cdf[i] );
        }
    }

    void MigrationInfoVector::PickMigrationStep( RANDOMBASE* pRNG,
                                                 IIndividualHumanContext *traveler, 
                                                 float migration_rate_modifier,
                                                 suids::suid& destination, 
                                                 MigrationType::Enum& migration_type, 
                                                 float& timeUntilTrip ) 
    {
        MigrationInfoFixedRate::PickMigrationStep( pRNG, traveler, migration_rate_modifier, destination, migration_type, timeUntilTrip );

        // ------------------------------------------------------------------
        // --- if the destination is the current node, then the selection
        // --- was to stay put.  If this is the choice, then we don't want to
        // --- return anything.
        // ------------------------------------------------------------------
        if( destination == m_ThisNodeId )
        {
            destination = suids::nil_suid();
            migration_type = MigrationType::NO_MIGRATION;
            timeUntilTrip = -1.0;
        }
    }

    int GetNodePopulation( const suids::suid& rNodeId, 
                           const std::string& rSpeciesID, 
                           IVectorSimulationContext* pivsc )
    {
        return pivsc->GetNodePopulation( rNodeId ) ;
    }

    int GetAvailableLarvalHabitat( const suids::suid& rNodeId, 
                                   const std::string& rSpeciesID, 
                                   IVectorSimulationContext* pivsc )
    {
        return pivsc->GetAvailableLarvalHabitat( rNodeId, rSpeciesID );
    }

    std::vector<float> MigrationInfoVector::GetRatios( const std::vector<suids::suid>& rReachableNodes, 
                                                       const std::string& rSpeciesID, 
                                                       IVectorSimulationContext* pivsc, 
                                                       tGetValueFunc getValueFunc )
    {
        // -----------------------------------
        // --- Find the total number of people
        // --- Find the total reachable and available larval habitat
        // -----------------------------------
        float total = 0.0 ;
        for( auto node_id : rReachableNodes )
        {
            total += getValueFunc( node_id, rSpeciesID, pivsc ) ;
        }

        std::vector<float> ratios ;
        for( auto node_id : rReachableNodes )
        {
            float pr = 0.0 ;
            if( total > 0.0 )
            {
                pr = getValueFunc( node_id, rSpeciesID, pivsc ) / total ;
            }
            ratios.push_back( pr );
        }
        return ratios;
    }

    void MigrationInfoVector::UpdateRates( const suids::suid& rThisNodeId, 
                                           const std::string& rSpeciesID, 
                                           IVectorSimulationContext* pivsc )
    {
        // ---------------------------------------------------------------------------------
        // --- If we want to factor in the likelihood that a vector will decide that
        // --- the grass is not greener on the other side, then we need to add "this/current"
        // --- node as a possible node to go to.
        // ---------------------------------------------------------------------------------
        if( (m_ModifierStayPut > 0.0) && (m_ReachableNodes.size() >0) && (m_ReachableNodes[0] != rThisNodeId) )
        {
            m_ThisNodeId = rThisNodeId ;
            m_ReachableNodes.insert( m_ReachableNodes.begin(), rThisNodeId );
            m_MigrationTypes.insert( m_MigrationTypes.begin(), MigrationType::LOCAL_MIGRATION );
            m_RawMigrationRate.insert( m_RawMigrationRate.begin(), 0.0 );
            m_RateCDF.insert( m_RateCDF.begin(), 0.0 );
        }

        // -------------------------------------------------------------------
        // --- Find the ratios of population and larval habitat (i.e. things
        // --- that influence the vectors migration).  These ratios will be used
        // --- in the equations that influence which node the vectors go to.
        // -------------------------------------------------------------------
        std::vector<float> pop_ratios     = GetRatios( m_ReachableNodes, rSpeciesID, pivsc, GetNodePopulation         );
        std::vector<float> habitat_ratios = GetRatios( m_ReachableNodes, rSpeciesID, pivsc, GetAvailableLarvalHabitat );

        // --------------------------------------------------------------------------
        // --- Determine the new rates by adding the rates from the files times
        // --- to the food and habitat adjusted rates.
        // --------------------------------------------------------------------------
        release_assert( m_RawMigrationRate.size() == m_ReachableNodes.size() );
        release_assert( m_RateCDF.size()          == m_ReachableNodes.size() );
        release_assert( m_RateCDF.size()          == pop_ratios.size()      );
        release_assert( m_RateCDF.size()          == habitat_ratios.size()  );

        float tmp_totalrate = 0.0;
        for( int i = 0; i < m_RateCDF.size(); i++)
        {
            tmp_totalrate += m_RawMigrationRate[i] ; // need this to be the raw rate

            m_RateCDF[i] = CalculateModifiedRate( m_ReachableNodes[i], 
                                                  m_RawMigrationRate[i], 
                                                  pop_ratios[i], 
                                                  habitat_ratios[i] ) ;
        }

        NormalizeRates( m_RateCDF, m_TotalRate );

        // -----------------------------------------------------------------------------------
        // --- We want to use the rate from the files instead of the value changed due to the 
        // --- food modifier.  If we don't do this we get much less migration than desired.
        // -----------------------------------------------------------------------------------
        m_TotalRate = tmp_totalrate;
    }

    float MigrationInfoVector::CalculateModifiedRate( const suids::suid& rNodeId,
                                                      float rawRate, 
                                                      float populationRatio, 
                                                      float habitatRatio )
    {
        // --------------------------------------------------------------------------
        // --- Determine the probability that the mosquito will not migrate because
        // --- there is enough food or habitat in there current node
        // --------------------------------------------------------------------------
        float sp = 1.0 ;
        if( (m_ModifierStayPut > 0.0) && (rNodeId == m_ThisNodeId) )
        {
            sp = m_ModifierStayPut ;
        }

        // ---------------------------------------------------------------------------------
        // --- 10/16/2015 Jaline says that research shows that vectors don't necessarily go
        // --- to houses with more people, but do go to places with people versus no people.
        // --- Hence, 1 => go to node with people, 0 => avoid nodes without people.
        // ---------------------------------------------------------------------------------
        float pr = populationRatio ;
        if( pr > 0.0 )
        {
            pr = 1.0 ;
        }

        float rate = 0.0 ;
        switch( m_ModifierEquation )
        {
            case ModiferEquationType::LINEAR:
                rate = rawRate + (sp * m_ModifierFood * pr) + (sp * m_ModifierHabitat * habitatRatio) ;
                break;
            case ModiferEquationType::EXPONENTIAL:
                {
                    // ------------------------------------------------------------
                    // --- The -1 allows for values between 0 and 1.  Otherwise,
                    // --- the closer we got to zero the more our get closer to 1.
                    // ------------------------------------------------------------
                    float fm = 0.0 ;
                    if( m_ModifierFood > 0.0 )
                    {
                        fm =  exp( sp * m_ModifierFood * pr ) - 1.0f;
                    }
                    float hm = 0.0 ;
                    if( m_ModifierHabitat > 0.0 )
                    {
                        hm = exp( sp * m_ModifierHabitat * habitatRatio ) - 1.0f ;
                    }
                    rate = rawRate + fm + hm ;
                }
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, MODIFIER_EQUATION_NAME, m_ModifierEquation, ModiferEquationType::pairs::lookup_key( m_ModifierEquation ) );
        }

        return rate ;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryVector
    // ------------------------------------------------------------------------
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Migration.Vector,MigrationInfoFactoryVector)
    BEGIN_QUERY_INTERFACE_DERIVED(MigrationInfoFactoryVector,MigrationInfoFactoryFile)
    END_QUERY_INTERFACE_DERIVED(MigrationInfoFactoryVector,MigrationInfoFactoryFile)

    MigrationInfoFactoryVector::MigrationInfoFactoryVector()
    : MigrationInfoFactoryFile()
    , m_InfoFileListVector()
    , m_IsVectorMigrationEnabled( false )
    , m_ModifierEquation( ModiferEquationType::EXPONENTIAL )
    , m_ModifierHabitat(0.0)
    , m_ModifierFood(0.0)
    , m_ModifierStayPut(0.0)
    {
    }

    MigrationInfoFactoryVector::~MigrationInfoFactoryVector()
    {
        for( auto mig_file : m_InfoFileListVector )
        {
            delete mig_file;
        }
        m_InfoFileListVector.clear();
    }

    void MigrationInfoFactoryVector::CreateInfoFileList()
    {
        MigrationInfoFactoryFile::CreateInfoFileList();

        m_InfoFileListVector.push_back( new MigrationInfoFile( MigrationType::LOCAL_MIGRATION,    MAX_LOCAL_MIGRATION_DESTINATIONS    ) );
        m_InfoFileListVector.push_back( nullptr );
        m_InfoFileListVector.push_back( new MigrationInfoFile( MigrationType::REGIONAL_MIGRATION, MAX_REGIONAL_MIGRATION_DESTINATIONS ) );
        m_InfoFileListVector.push_back( nullptr );
        m_InfoFileListVector.push_back( nullptr );
    }

    void MigrationInfoFactoryVector::InitializeInfoFileList( const Configuration* config )
    {
        MigrationInfoFactoryFile::InitializeInfoFileList( config );

        initConfigTypeMap( ENABLE_VECTOR_MIGRATION_NAME, &m_IsVectorMigrationEnabled, Enable_Vector_Migration_DESC_TEXT, false );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! One should not typically get the value of a parameter as in this 'if' check.
        // !!! I did it because it was the only way to avoid needing to read in all of these parameters.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        {
            initConfig( MODIFIER_EQUATION_NAME, 
                        m_ModifierEquation, 
                        config, 
                        MetadataDescriptor::Enum(MODIFIER_EQUATION_NAME, Vector_Migration_Modifier_Equation_DESC_TEXT, MDD_ENUM_ARGS(ModiferEquationType)), "Enable_Vector_Migration" ); 

            initConfigTypeMap( "Enable_Vector_Migration_Local",      &(m_InfoFileListVector[0]->m_IsEnabled), Enable_Vector_Migration_Local_DESC_TEXT,    false, "Enable_Vector_Migration" );
            initConfigTypeMap( "Enable_Vector_Migration_Regional",   &(m_InfoFileListVector[2]->m_IsEnabled), Enable_Vector_Migration_Regional_DESC_TEXT, false, "Enable_Vector_Migration" ); 

            initConfigTypeMap( "Vector_Migration_Filename_Local",    &(m_InfoFileListVector[0]->m_Filename),  Vector_Migration_Filename_Local_DESC_TEXT, "UNSPECIFIED_FILE", "Enable_Vector_Migration_Local"    );
            initConfigTypeMap( "Vector_Migration_Filename_Regional", &(m_InfoFileListVector[2]->m_Filename),  Vector_Migration_Filename_Regional_DESC_TEXT, "UNSPECIFIED_FILE", "Enable_Vector_Migration_Regional" );

            initConfigTypeMap( "x_Vector_Migration_Local",           &(m_InfoFileListVector[0]->m_xModifier), x_Vector_Migration_Local_DESC_TEXT,    0.0f, FLT_MAX, 1.0f, "Enable_Vector_Migration_Local" );
            initConfigTypeMap( "x_Vector_Migration_Regional",        &(m_InfoFileListVector[2]->m_xModifier), x_Vector_Migration_Regional_DESC_TEXT, 0.0f, FLT_MAX, 1.0f, "Enable_Vector_Migration_Regional" );

            initConfigTypeMap( "Vector_Migration_Habitat_Modifier",  &m_ModifierHabitat,  Vector_Migration_Habitat_Modifier_DESC_TEXT,  0.0f, FLT_MAX, 0.0f, "Enable_Vector_Migration" );
            initConfigTypeMap( "Vector_Migration_Food_Modifier",     &m_ModifierFood,     Vector_Migration_Food_Modifier_DESC_TEXT,     0.0f, FLT_MAX, 0.0f, "Enable_Vector_Migration" );
            initConfigTypeMap( "Vector_Migration_Stay_Put_Modifier", &m_ModifierStayPut,  Vector_Migration_Stay_Put_Modifier_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Enable_Vector_Migration" );
        }

        m_InfoFileListVector[0]->SetEnableParameterName( "Enable_Vector_Migration_Local"    );
        m_InfoFileListVector[2]->SetEnableParameterName( "Enable_Vector_Migration_Regional" );

        m_InfoFileListVector[0]->SetFilenameParameterName( "Vector_Migration_Filename_Local"    );
        m_InfoFileListVector[2]->SetFilenameParameterName( "Vector_Migration_Filename_Regional" );
    }

    bool MigrationInfoFactoryVector::IsVectorMigrationEnabled() const
    {
        return m_IsVectorMigrationEnabled;
    }

    void MigrationInfoFactoryVector::Initialize( const ::Configuration* config, const string& idreference )
    {
        MigrationInfoFactoryFile::Initialize( config, idreference );

        for( int i = 0 ; i < m_InfoFileListVector.size() ; i++ )
        {
            if( m_InfoFileListVector[i] != nullptr )
            {
                if( m_InfoFileListVector[i]->m_IsEnabled )
                {
                    m_InfoFileListVector[i]->Initialize( idreference );
                }
            }
        }
    }

    IMigrationInfoVector* MigrationInfoFactoryVector::CreateMigrationInfoVector( 
        INodeContext *pParentNode, 
        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        bool is_fixed_rate = true ;
        std::vector<std::vector<MigrationRateData>> rate_data = GetRateData( pParentNode, rNodeIdSuidMap, m_InfoFileListVector, &is_fixed_rate );

        MigrationInfoVector* new_migration_info = _new_ MigrationInfoVector( pParentNode,
                                                                             m_ModifierEquation,
                                                                             m_ModifierHabitat,
                                                                             m_ModifierFood,
                                                                             m_ModifierStayPut );
        new_migration_info->Initialize( rate_data );

        return new_migration_info ;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryVectorDefault
    // ------------------------------------------------------------------------

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Migration.VectorDefault,MigrationInfoFactoryVectorDefault)
    BEGIN_QUERY_INTERFACE_DERIVED(MigrationInfoFactoryVectorDefault,MigrationInfoFactoryDefault)
    END_QUERY_INTERFACE_DERIVED(MigrationInfoFactoryVectorDefault,MigrationInfoFactoryDefault)

    MigrationInfoFactoryVectorDefault::MigrationInfoFactoryVectorDefault( int torusSize )
    : MigrationInfoFactoryDefault( torusSize )
    , m_IsVectorMigrationEnabled( false )
    , m_xLocalModifierVector(1.0)
    {
    }

    MigrationInfoFactoryVectorDefault::MigrationInfoFactoryVectorDefault()
    : MigrationInfoFactoryDefault( 10 )
    , m_IsVectorMigrationEnabled( false )
    , m_xLocalModifierVector(1.0)
    {
    }

    MigrationInfoFactoryVectorDefault::~MigrationInfoFactoryVectorDefault()
    {
    }

    bool MigrationInfoFactoryVectorDefault::Configure( const Configuration* config )
    {
        initConfigTypeMap(ENABLE_VECTOR_MIGRATION_NAME, &m_IsVectorMigrationEnabled, Enable_Vector_Migration_DESC_TEXT, false );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! One should not typically get the value of a parameter as in this 'if' check.
        // !!! I did it because it was the only way to avoid needint to read in all of these parameters.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if( JsonConfigurable::_dryrun || 
            (config->Exist( ENABLE_VECTOR_MIGRATION_NAME ) && (int((*config)[ENABLE_VECTOR_MIGRATION_NAME].As<json::Number>()) == 1)) )
        {
            initConfigTypeMap( "x_Vector_Migration_Local", &m_xLocalModifierVector, x_Local_Migration_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        }

        bool ret = MigrationInfoFactoryDefault::Configure( config );
        return ret;
    }

    IMigrationInfoVector* MigrationInfoFactoryVectorDefault::CreateMigrationInfoVector( INodeContext *pParentNode, 
                                                                                        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        if( IsVectorMigrationEnabled() )
        {
            std::vector<std::vector<MigrationRateData>> rate_data = GetRateData( pParentNode, rNodeIdSuidMap, m_xLocalModifierVector );

            MigrationInfoVector* new_migration_info = _new_ MigrationInfoVector( pParentNode, 
                                                                                 ModiferEquationType::LINEAR,
                                                                                 1.0,
                                                                                 1.0,
                                                                                 1.0 );
            new_migration_info->Initialize( rate_data );

            return new_migration_info;
        }
        else
        {
            MigrationInfoNullVector* null_info = new MigrationInfoNullVector();
            return null_info;
        }
    }

    bool MigrationInfoFactoryVectorDefault::IsVectorMigrationEnabled() const
    {
        return m_IsVectorMigrationEnabled;
    }

}
