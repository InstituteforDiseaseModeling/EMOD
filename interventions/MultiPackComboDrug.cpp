
#include "stdafx.h"
#include "MultiPackComboDrug.h"
#include "DrugModelAntiMalarial.h"

#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "MalariaDrugTypeParameters.h"
#include "MalariaContexts.h"

SETUP_LOGGING( "MultiPackComboDrug" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(MultiPackComboDrug, AntimalarialDrug)
    END_QUERY_INTERFACE_DERIVED(MultiPackComboDrug, AntimalarialDrug)
    IMPLEMENT_FACTORY_REGISTERED(MultiPackComboDrug)

    MultiPackComboDrug::MultiPackComboDrug()
        : AntimalarialDrug()
        , m_DosesNames()
        , m_DoseInterval( 1.0f )
        , m_CurrentDoseIndex( -1 ) // -1 = have not take a dose yet
        , m_NamesOfDrugsTaken()
        , m_DrugsTaken()
    {
    }

    MultiPackComboDrug::MultiPackComboDrug( const MultiPackComboDrug& rThat )
        : AntimalarialDrug( rThat )
        , m_DosesNames( rThat.m_DosesNames )
        , m_DoseInterval( rThat.m_DoseInterval )
        , m_CurrentDoseIndex( rThat.m_CurrentDoseIndex )
        , m_NamesOfDrugsTaken( rThat.m_NamesOfDrugsTaken )
        , m_DrugsTaken()
    {
        for( auto entry : rThat.m_DrugsTaken )
        {
            DrugModelAntiMalarial* p_that_model = entry.second;
            DrugModelAntiMalarial* p_new_model = dynamic_cast<DrugModelAntiMalarial*>(p_that_model->Clone());
            this->m_DrugsTaken.insert( std::make_pair( p_new_model->GetDrugName(), p_new_model ) );
        }
    }

    MultiPackComboDrug::~MultiPackComboDrug()
    {
        for( auto entry : m_DrugsTaken )
        {
            delete entry.second;
        }
        m_DrugsTaken.clear();
    }

    void MultiPackComboDrug::ConfigureDrugType( const Configuration *intputJson )
    {
        const jsonConfigurable::tDynamicStringSet* p_name_set = nullptr;
        if( !JsonConfigurable::_dryrun )
        {
            p_name_set = &(MalariaDrugTypeCollection::GetInstance()->GetDrugNames());
        }

        initConfigTypeMap( "Doses",         &m_DosesNames,   MPCDrug_Doses_DESC_TEXT,        nullptr, *p_name_set );
        initConfigTypeMap( "Dose_Interval", &m_DoseInterval, MPCDrug_DoseInterval_DESC_TEXT, 0.0f, 100000.0f, 1.0f );
    }

    void MultiPackComboDrug::CheckConfigureDrugType( const Configuration *inputJson )
    {
        if( m_DosesNames.empty() || !inputJson->Exist( "Doses" ) )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "'Doses' was not defined and it is a required parameter." );
        }

        delete p_drug_model;
        p_drug_model = nullptr;
    }

    const std::string& MultiPackComboDrug::GetDrugName() const
    {
        return m_NamesOfDrugsTaken;
    }

    float MultiPackComboDrug::GetDrugCurrentEfficacy() const
    {
        if( m_DrugsTaken.size() == 0 )
        {
            return 0.0f;
        }

        // eff = 1 - (1 - drugA)(1 - drugB)...
        float combined_efficacies = 1.0;
        for( auto& entry : m_DrugsTaken )
        {
            combined_efficacies *= (1.0f - entry.second->GetDrugCurrentEfficacy());
        }
        float efficacy = 1.0 - combined_efficacies;
        return efficacy;
    }

    float MultiPackComboDrug::GetDrugCurrentConcentration() const
    {
        if( m_DrugsTaken.size() == 0 )
        {
            return 0.0f;
        }

        float concentration = 0.0;
        for( auto& entry : m_DrugsTaken )
        {
            concentration += static_cast<DrugModelAntiMalarial*>(entry.second)->GetDrugCurrentConcentration();
        }
        return concentration;
    }


    float MultiPackComboDrug::get_drug_IRBC_killrate( const IStrainIdentity& rStrain )
    {
        float killrate = 0.0;
        for( auto& entry : m_DrugsTaken )
        {
            killrate += static_cast<DrugModelAntiMalarial*>(entry.second)->get_drug_IRBC_killrate( rStrain );
        }
        return killrate;
    }

    float MultiPackComboDrug::get_drug_hepatocyte( const IStrainIdentity& rStrain )
    {
        float hepatocyte = 0.0;
        for( auto& entry : m_DrugsTaken )
        {
            hepatocyte += static_cast<DrugModelAntiMalarial*>(entry.second)->get_drug_hepatocyte( rStrain );
        }
        return hepatocyte;
    }

    float MultiPackComboDrug::get_drug_gametocyte02( const IStrainIdentity& rStrain )
    {
        float gametocyte02 = 0.0;
        for( auto& entry : m_DrugsTaken )
        {
            gametocyte02 += static_cast<DrugModelAntiMalarial*>(entry.second)->get_drug_gametocyte02( rStrain );
        }
        return gametocyte02;
    }

    float MultiPackComboDrug::get_drug_gametocyte34( const IStrainIdentity& rStrain )
    {
        float gametocyte34 = 0.0;
        for( auto& entry : m_DrugsTaken )
        {
            gametocyte34 += static_cast<DrugModelAntiMalarial*>(entry.second)->get_drug_gametocyte34( rStrain );
        }
        return gametocyte34;
    }

    float MultiPackComboDrug::get_drug_gametocyteM( const IStrainIdentity& rStrain )
    {
        float gametocyteM = 0.0;
        for( auto& entry : m_DrugsTaken )
        {
            gametocyteM += static_cast<DrugModelAntiMalarial*>(entry.second)->get_drug_gametocyteM( rStrain );
        }
        return gametocyteM;
    }

    int MultiPackComboDrug::GetNumDoses() const
    {
        return m_DosesNames.size();
    }

    float MultiPackComboDrug::GetDoseInterval() const
    {
        return m_DoseInterval;
    }

    void MultiPackComboDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    {
        // WARNING: These two lines are duplicated from AntimalarialDrug
        remaining_doses = GetNumDoses();
        time_between_doses = GetDoseInterval();
        m_CurrentDoseIndex = -1; // -1 = have not take a dose yet
    }

    void MultiPackComboDrug::TakeDose( float dt, RANDOMBASE* pRNG, IIndividualHumanInterventionsContext * ivc )
    {
        m_CurrentDoseIndex = GetNumDoses() - remaining_doses;

        // ---------------------------------------------------------------------------
        // --- Only take the dose of the drugs specified in the current m_DosesNames.
        // --- There maybe other drugs taken, but only taking the drugs for this dose.
        // ---------------------------------------------------------------------------
        for( auto drug_name : m_DosesNames[ m_CurrentDoseIndex ] )
        {
            if( m_DrugsTaken.count( drug_name ) == 0 )
            {
                // ---------------------------------------------------
                // --- Didn't find drug so create model and configure
                // --- We only need to create the drug model if it has
                // --- not been created yet
                // ---------------------------------------------------
                const MalariaDrugTypeParameters& r_params = MalariaDrugTypeCollection::GetInstance()->GetDrug( drug_name );

                DrugModelAntiMalarial* p_new_model = new DrugModelAntiMalarial( drug_name, &r_params );

                m_DrugsTaken.insert( std::make_pair( p_new_model->GetDrugName(), p_new_model ) );
                p_new_model->ConfigureDrugTreatment( ivc );
            }

            DrugModelAntiMalarial* p_model = m_DrugsTaken[ drug_name ];
            p_model->TakeDose( dt, pRNG );
        }

        UpdateDrugName();
    }

    void MultiPackComboDrug::DecayAndUpdateEfficacy( float dt )
    {
        for( auto& entry : m_DrugsTaken )
        {
            entry.second->DecayAndUpdateEfficacy( dt );
        }
    }

    void MultiPackComboDrug::ResetForNextDose(float dt)
    {
        AntimalarialDrug::ResetForNextDose( dt );
    }

    void MultiPackComboDrug::UpdateDrugName()
    {
        int i = 0;
        m_NamesOfDrugsTaken = "";
        for( auto& entry : m_DrugsTaken )
        {
            m_NamesOfDrugsTaken += entry.first;
            if( (i + 1) < m_DrugsTaken.size() )
            {
                m_NamesOfDrugsTaken += "+";
            }
            ++i;
        }
    }

    REGISTER_SERIALIZABLE(MultiPackComboDrug);

    void MultiPackComboDrug::serialize(IArchive& ar, MultiPackComboDrug* obj)
    {
        AntimalarialDrug::serialize(ar, obj);
        MultiPackComboDrug& drug = *obj;

        ar.labelElement( "m_DosesNames"        ) & drug.m_DosesNames;
        ar.labelElement( "m_DoseInterval"      ) & drug.m_DoseInterval;
        ar.labelElement( "m_CurrentDoseIndex"  ) & drug.m_CurrentDoseIndex;
        ar.labelElement( "m_NamesOfDrugsTaken" ) & drug.m_NamesOfDrugsTaken;
        ar.labelElement( "m_DrugsTaken"        ) & drug.m_DrugsTaken;
    }
}
