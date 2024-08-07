
#include "stdafx.h"
#include "MigrationInfoVector.h"
#include "INodeContext.h"
#include "VectorContexts.h"
#include "SimulationEnums.h"


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
    // --- MigrationInfoFixedRateVector
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(MigrationInfoFixedRateVector, MigrationInfoFixedRate)
    END_QUERY_INTERFACE_DERIVED(MigrationInfoFixedRateVector, MigrationInfoFixedRate)

    MigrationInfoFixedRateVector::MigrationInfoFixedRateVector( INodeContext * _parent,
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

    MigrationInfoFixedRateVector::~MigrationInfoFixedRateVector()
    {
    }

    void MigrationInfoFixedRateVector::Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData )
    {
        MigrationInfoFixedRate::Initialize( rRateData );
    }

    void MigrationInfoFixedRateVector::SaveRawRates( std::vector<float>& r_rate_cdf, Gender::Enum gender)
    {
        // ---------------------------------------------------------
        // --- Keep the un-normalized rates so we can multiply them
        // --- times our food adjusted rates.
        // ---------------------------------------------------------
        // Fixed rate is always for both genders, thus ignoring gender
        m_RawMigrationRate.clear();
        for( int i = 0; i < r_rate_cdf.size(); i++)
        {
            m_RawMigrationRate.push_back( r_rate_cdf[i] );
        }
    }

    void MigrationInfoFixedRateVector::PickMigrationStep( RANDOMBASE* pRNG,
                                                          IIndividualHumanEventContext *traveler, 
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

    Gender::Enum MigrationInfoFixedRateVector::ConvertVectorGender(VectorGender::Enum gender) const
    {
        return (gender == VectorGender::VECTOR_FEMALE) ? Gender::FEMALE : Gender::MALE;
    }

    void MigrationInfoFixedRateVector::CalculateRates(VectorGender::Enum vector_gender)
    {
        //No need to do anything, CalculateRates ran at Initialize for MigrationInforFixedRate
    }

    std::vector<float> MigrationInfoFixedRateVector::GetRatios( const std::vector<suids::suid>& rReachableNodes,
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

    void MigrationInfoFixedRateVector::UpdateRates( const suids::suid& rThisNodeId,
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

    float MigrationInfoFixedRateVector::CalculateModifiedRate( const suids::suid& rNodeId,
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
    // --- MigrationInfoAgeAndGenderVector
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(MigrationInfoAgeAndGenderVector, MigrationInfoAgeAndGender)
        END_QUERY_INTERFACE_DERIVED(MigrationInfoAgeAndGenderVector, MigrationInfoAgeAndGender)

        MigrationInfoAgeAndGenderVector::MigrationInfoAgeAndGenderVector(INodeContext* _parent,
            ModiferEquationType::Enum equation,
            float habitatModifier,
            float foodModifier,
            float stayPutModifier)
        : MigrationInfoAgeAndGender(_parent, false)
        , m_RawMigrationRatesVectorGender()
        , m_TotalRatesVectorGender()
        , m_RateCDFVectorGender()
        , m_ThisNodeId(suids::nil_suid())
        , m_ModifierEquation(equation)
        , m_ModifierHabitat(habitatModifier)
        , m_ModifierFood(foodModifier)
        , m_ModifierStayPut(stayPutModifier)
    {
        m_RawMigrationRatesVectorGender = { {0}, {0} }; //we need for indices to exist to add data
    }

    MigrationInfoAgeAndGenderVector::~MigrationInfoAgeAndGenderVector()
    {
    }

    void MigrationInfoAgeAndGenderVector::Initialize(const std::vector<std::vector<MigrationRateData>>& rRateData)
    {
        MigrationInfoAgeAndGender::Initialize(rRateData);
    }

    void MigrationInfoAgeAndGenderVector::CalculateRates(VectorGender::Enum vector_gender)
    {
        Gender::Enum human_equivalent = ConvertVectorGender(vector_gender);
        MigrationInfoAgeAndGender::CalculateRates(human_equivalent, 0);
    }


    void MigrationInfoAgeAndGenderVector::SaveRawRates(std::vector<float>& r_rate_cdf, Gender::Enum gender)
    {
        // ---------------------------------------------------------
        // --- Keep the un-normalized rates so we can multiply them
        // --- times our food adjusted rates.
        // ---------------------------------------------------------
        int vector_gender_index = int(gender == Gender::MALE ? VectorGender::VECTOR_MALE : VectorGender::VECTOR_FEMALE);
        m_RawMigrationRatesVectorGender[vector_gender_index].clear();
        for (int i = 0; i < r_rate_cdf.size(); i++)
        {
            m_RawMigrationRatesVectorGender[vector_gender_index].push_back(r_rate_cdf[i]);
        }
    }

    Gender::Enum MigrationInfoAgeAndGenderVector::ConvertVectorGender(VectorGender::Enum vector_gender) const
    {
        return (vector_gender == VectorGender::VECTOR_FEMALE) ? Gender::FEMALE : Gender::MALE;
    }


    std::vector<float> MigrationInfoAgeAndGenderVector::GetRatios(const std::vector<suids::suid>& rReachableNodes,
        const std::string& rSpeciesID,
        IVectorSimulationContext* pivsc,
        tGetValueFunc getValueFunc)
    {
        // -----------------------------------
        // --- Find the total number of people
        // --- Find the total reachable and available larval habitat
        // -----------------------------------
        float total = 0.0;
        for (auto node_id : rReachableNodes)
        {
            total += getValueFunc(node_id, rSpeciesID, pivsc);
        }

        std::vector<float> ratios;
        for (auto node_id : rReachableNodes)
        {
            float pr = 0.0;
            if (total > 0.0)
            {
                pr = getValueFunc(node_id, rSpeciesID, pivsc) / total;
            }
            ratios.push_back(pr);
        }
        return ratios;
    }

    void MigrationInfoAgeAndGenderVector::UpdateRates(const suids::suid& rThisNodeId,
                                                      const std::string& rSpeciesID,
                                                      IVectorSimulationContext* pivsc)
    {
        // ---------------------------------------------------------------------------------
        // --- If we want to factor in the likelihood that a vector will decide that
        // --- the grass is not greener on the other side, then we need to add "this/current"
        // --- node as a possible node to go to.
        // ---------------------------------------------------------------------------------
        
        // recalculating for female vector population only
        
        Gender::Enum human_equivalent = ConvertVectorGender(VectorGender::VECTOR_FEMALE);
        int female_vectors_index = (int)VectorGender::VECTOR_FEMALE;
        m_ReachableNodes = GetReachableNodes(human_equivalent);
        m_MigrationTypes = GetMigrationTypes(human_equivalent);
        std::vector<float> m_RawMigrationRate = m_RawMigrationRatesVectorGender[female_vectors_index];

        // after this it is the same as MigrationFixedRateVector, can we.. use that somehow? a shared function?
        if ((m_ModifierStayPut > 0.0) && (m_ReachableNodes.size() > 0) && (m_ReachableNodes[0] != rThisNodeId))
        {
            m_ThisNodeId = rThisNodeId;
            m_ReachableNodes.insert(m_ReachableNodes.begin(), rThisNodeId);
            m_MigrationTypes.insert(m_MigrationTypes.begin(), MigrationType::LOCAL_MIGRATION);
            m_RawMigrationRate.insert(m_RawMigrationRate.begin(), 0.0);
            m_RateCDF.insert(m_RateCDF.begin(), 0.0);
        }

        // -------------------------------------------------------------------
        // --- Find the ratios of population and larval habitat (i.e. things
        // --- that influence the vectors migration).  These ratios will be used
        // --- in the equations that influence which node the vectors go to.
        // -------------------------------------------------------------------
        std::vector<float> pop_ratios = GetRatios(m_ReachableNodes, rSpeciesID, pivsc, GetNodePopulation);
        std::vector<float> habitat_ratios = GetRatios(m_ReachableNodes, rSpeciesID, pivsc, GetAvailableLarvalHabitat);

        // --------------------------------------------------------------------------
        // --- Determine the new rates by adding the rates from the files times
        // --- to the food and habitat adjusted rates.
        // --------------------------------------------------------------------------
        release_assert(m_RawMigrationRate.size() == m_ReachableNodes.size());
        release_assert(m_RateCDF.size() == m_ReachableNodes.size());
        release_assert(m_RateCDF.size() == pop_ratios.size());
        release_assert(m_RateCDF.size() == habitat_ratios.size());

        float tmp_totalrate = 0.0;
        for (int i = 0; i < m_RateCDF.size(); i++)
        {
            tmp_totalrate += m_RawMigrationRate[i]; // need this to be the raw rate

            m_RateCDF[i] = CalculateModifiedRate(m_ReachableNodes[i],
                m_RawMigrationRate[i],
                pop_ratios[i],
                habitat_ratios[i]);
        }

        NormalizeRates(m_RateCDF, m_TotalRate);

        // -----------------------------------------------------------------------------------
        // --- We want to use the rate from the files instead of the value changed due to the 
        // --- food modifier.  If we don't do this we get much less migration than desired.
        // -----------------------------------------------------------------------------------
        m_TotalRate = tmp_totalrate;

    }

    float MigrationInfoAgeAndGenderVector::CalculateModifiedRate(const suids::suid& rNodeId,
        float rawRate,
        float populationRatio,
        float habitatRatio)
    {
        // --------------------------------------------------------------------------
        // --- Determine the probability that the mosquito will not migrate because
        // --- there is enough food or habitat in there current node
        // --------------------------------------------------------------------------
        float sp = 1.0;
        if ((m_ModifierStayPut > 0.0) && (rNodeId == m_ThisNodeId))
        {
            sp = m_ModifierStayPut;
        }

        // ---------------------------------------------------------------------------------
        // --- 10/16/2015 Jaline says that research shows that vectors don't necessarily go
        // --- to houses with more people, but do go to places with people versus no people.
        // --- Hence, 1 => go to node with people, 0 => avoid nodes without people.
        // ---------------------------------------------------------------------------------
        float pr = populationRatio;
        if (pr > 0.0)
        {
            pr = 1.0;
        }

        float rate = 0.0;
        switch (m_ModifierEquation)
        {
        case ModiferEquationType::LINEAR:
            rate = rawRate + (sp * m_ModifierFood * pr) + (sp * m_ModifierHabitat * habitatRatio);
            break;
        case ModiferEquationType::EXPONENTIAL:
        {
            // ------------------------------------------------------------
            // --- The -1 allows for values between 0 and 1.  Otherwise,
            // --- the closer we got to zero the more our get closer to 1.
            // ------------------------------------------------------------
            float fm = 0.0;
            if (m_ModifierFood > 0.0)
            {
                fm = exp(sp * m_ModifierFood * pr) - 1.0f;
            }
            float hm = 0.0;
            if (m_ModifierHabitat > 0.0)
            {
                hm = exp(sp * m_ModifierHabitat * habitatRatio) - 1.0f;
            }
            rate = rawRate + fm + hm;
        }
        break;
        default:
            throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, MODIFIER_EQUATION_NAME, m_ModifierEquation, ModiferEquationType::pairs::lookup_key(m_ModifierEquation));
        }

        return rate;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryVector
    // ------------------------------------------------------------------------
    MigrationInfoFactoryVector::MigrationInfoFactoryVector( bool enableVectorMigration )
    : m_InfoFileVector( MigrationType::LOCAL_MIGRATION, 8 )
    , m_ModifierEquation( ModiferEquationType::EXPONENTIAL )
    , m_ModifierHabitat(0.0)
    , m_ModifierFood(0.0)
    , m_ModifierStayPut(0.0)
    {
        m_InfoFileVector.m_IsEnabled = enableVectorMigration;
    }

    MigrationInfoFactoryVector::~MigrationInfoFactoryVector()
    {
    }

    void MigrationInfoFactoryVector::ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config )
    {
        pParent->initConfig( MODIFIER_EQUATION_NAME, 
                             m_ModifierEquation, 
                             config, 
                             MetadataDescriptor::Enum( MODIFIER_EQUATION_NAME,
                                                       Vector_Migration_Modifier_Equation_DESC_TEXT,
                                                       MDD_ENUM_ARGS(ModiferEquationType)),
                             "Enable_Vector_Migration" ); 

        pParent->initConfigTypeMap( "Vector_Migration_Filename",          &(m_InfoFileVector.m_Filename),  Vector_Migration_Filename_DESC_TEXT, "UNSPECIFIED_FILE",  "Enable_Vector_Migration" );
        pParent->initConfigTypeMap( "x_Vector_Migration"       ,          &(m_InfoFileVector.m_xModifier), x_Vector_Migration_DESC_TEXT,        0.0f, FLT_MAX, 1.0f, "Enable_Vector_Migration" );
        pParent->initConfigTypeMap( "Vector_Migration_Habitat_Modifier",  &m_ModifierHabitat,  Vector_Migration_Habitat_Modifier_DESC_TEXT,  0.0f, FLT_MAX, 0.0f, "Enable_Vector_Migration" );
        pParent->initConfigTypeMap( "Vector_Migration_Food_Modifier",     &m_ModifierFood,     Vector_Migration_Food_Modifier_DESC_TEXT,     0.0f, FLT_MAX, 0.0f, "Enable_Vector_Migration" );
        pParent->initConfigTypeMap( "Vector_Migration_Stay_Put_Modifier", &m_ModifierStayPut,  Vector_Migration_Stay_Put_Modifier_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Enable_Vector_Migration" );

        m_InfoFileVector.SetEnableParameterName( "Enable_Vector_Migration" );
        m_InfoFileVector.SetFilenameParameterName( "Vector_Migration_Filename" );
    }

    IMigrationInfoVector* MigrationInfoFactoryVector::CreateMigrationInfoVector( 
        const std::string& idreference,
        INodeContext *pParentNode, 
        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        IMigrationInfoVector* p_new_migration_info; // = nullptr;
        if( !m_InfoFileVector.IsInitialized() )
        {
			m_InfoFileVector.Initialize( idreference );
	        std::vector<MigrationInfoFile*> info_file_list;
	        info_file_list.push_back( &m_InfoFileVector );
	        info_file_list.push_back( nullptr );
	        info_file_list.push_back( nullptr );
	        info_file_list.push_back( nullptr );
	        info_file_list.push_back( nullptr );
	        bool is_fixed_rate = true ;
			std::vector<std::vector<MigrationRateData>> rate_data = MigrationInfoFactoryFile::GetRateData( pParentNode,
                                                                                                       rNodeIdSuidMap,
                                                                                                       info_file_list,
                                                                                                       &is_fixed_rate );
			

            if (is_fixed_rate)
            {
                MigrationInfoFixedRateVector* new_migration_info = _new_ MigrationInfoFixedRateVector(pParentNode,
                    m_ModifierEquation,
                    m_ModifierHabitat,
                    m_ModifierFood,
                    m_ModifierStayPut);
                new_migration_info->Initialize(rate_data);
                p_new_migration_info = new_migration_info;
            }
            else
            {
                MigrationInfoAgeAndGenderVector* new_migration_info = _new_ MigrationInfoAgeAndGenderVector(pParentNode,
                    m_ModifierEquation,
                    m_ModifierHabitat,
                    m_ModifierFood,
                    m_ModifierStayPut);
                new_migration_info->Initialize(rate_data);
                p_new_migration_info = new_migration_info;
            }
        }
        else
        {
            MigrationInfoNullVector* new_migration_info = new MigrationInfoNullVector();
            p_new_migration_info = new_migration_info;
        }

        return p_new_migration_info;
    }

    // ------------------------------------------------------------------------
    // --- MigrationInfoFactoryVectorDefault
    // ------------------------------------------------------------------------

    MigrationInfoFactoryVectorDefault::MigrationInfoFactoryVectorDefault( bool enableVectorMigration,
                                                                          int torusSize )
        : m_IsVectorMigrationEnabled( enableVectorMigration )
        , m_TorusSize( torusSize )
    {
    }

    MigrationInfoFactoryVectorDefault::~MigrationInfoFactoryVectorDefault()
    {
    }

    IMigrationInfoVector* MigrationInfoFactoryVectorDefault::CreateMigrationInfoVector( 
        const std::string& idreference,
        INodeContext *pParentNode, 
        const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        if( m_IsVectorMigrationEnabled )
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! I don't know what to do about this.
            // !!! Fixing it to 1 so we can move forward.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            float x_local_modifier = 1.0;

            std::vector<std::vector<MigrationRateData>> rate_data = MigrationInfoFactoryDefault::GetRateData( pParentNode, 
                                                                                                              rNodeIdSuidMap,
                                                                                                              m_TorusSize,
                                                                                                              x_local_modifier );

            MigrationInfoFixedRateVector* new_migration_info = _new_ MigrationInfoFixedRateVector( pParentNode,
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
}
