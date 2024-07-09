
#include "stdafx.h"

#include <iomanip>
#include "Debug.h"
#include "ReportHIVInfection.h"
#include "NodeHIV.h"
#include "SusceptibilityHIV.h"
#include "ISimulation.h"
#include "IIndividualHumanHIV.h"
#include "IInfectionHIV.h"
#include "IHIVInterventionsContainer.h"

SETUP_LOGGING( "ReportHIVInfection" )

namespace Kernel 
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(ReportHIVInfection,ReportHIVInfection)

    ReportHIVInfection::ReportHIVInfection( const ISimulation * parent )
        : BaseTextReport( "ReportHIVInfection.csv" )
        , _parent( parent )
        , startYear(0.0)
        , stopYear(FLT_MAX)
    {
    }

    bool ReportHIVInfection::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Report_HIV_Infection_Start_Year", &startYear, Report_HIV_Infection_Start_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, MIN_YEAR );
        initConfigTypeMap( "Report_HIV_Infection_Stop_Year",  &stopYear,  Report_HIV_Infection_Stop_Year_DESC_TEXT,  MIN_YEAR, MAX_YEAR, MAX_YEAR );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret )
        {
            if( startYear >= stopYear )
            {
                 throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Report_HIV_Infection_Start_Year", startYear, "Report_HIV_Infection_Stop_Year", stopYear );
            }
        }

        return ret ;
    }

    bool ReportHIVInfection::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        // not enforcing simulation to be not null in constructor so one can create schema with it null
        release_assert( _parent );

        float current_year = _parent->GetSimulationTime().Year() ;
        bool collect_data = (startYear <= current_year) && (current_year < stopYear)  ;
        return collect_data ;
    }

    std::string ReportHIVInfection::GetHeader() const
    {
        std::stringstream header ;
        header << "Year,"
               << "Node_ID,"
               << "Id,"
               << "MCWeight,"
               << "Age,"
               << "Gender,"
               << "getProbMaternalTransmission,"
               << "TimeSinceHIV,"
               << "CD4count,"
               << "PrognosisCompletedFraction,"
               << "Prognosis,"
               << "Stage,"
               << "ViralLoad,"
               << "WHOStage,"

               << "Infectiousness,"
               << "ModAcquire,"
               << "ModTransmit,"
               << "ModMortality,"

               << "ArtStatus,"
               << "InfectivitySuppression,"
               << "DurationOnART,"
               << "ProbMaternalTransmissionModifier,"
               << "OnArtQuery,"

               << "CoInfectiveTransmissionFactor,"
               << "CoInfectiveAcquisitionFactor,"
               << "DebutAge,"
               << "IsCircumcised,"

               << "InterventionReducedAcquire,"
               << "InterventionReducedTransmit,"
               << "InterventionReducedMortality" ;

        return header.str();
    }


    void
    ReportHIVInfection::LogIndividualData(
        IIndividualHuman* individual
    )
    {
        IIndividualHumanHIV* hiv_individual = nullptr;
        if( individual->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHIV", "IndividualHuman" );
        }

        bool isInfected = hiv_individual->HasHIV();

        if( !isInfected )
            return;

        IIndividualHumanSTI* sti_individual = hiv_individual->GetIndividualHumanSTI();

        IDrugVaccineInterventionEffects *idvie = nullptr;
        if( s_OK != individual->GetInterventionsContext()->QueryInterface( GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie ) ) {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, 
                                           "individual->GetInterventionsContext()", 
                                           "IDrugVaccineInterventionEffects",
                                           "IIndividualHumanInterventionsContext" );
        }

        GetOutputStream() 
            << std::setprecision(10)
            << _parent->GetSimulationTime().Year()
            << "," << individual->GetParent()->GetExternalID()
            << "," << individual->GetSuid().data
            << "," << individual->GetMonteCarloWeight()
            << "," << individual->GetAge() / DAYSPERYEAR
            << "," << individual->GetGender() 
            << "," << individual->getProbMaternalTransmission()
            << "," << _parent->GetSimulationTime().time - hiv_individual->GetHIVInfection()->GetTimeInfected() 
            << "," << hiv_individual->GetHIVSusceptibility()->GetCD4count() 
            << "," << hiv_individual->GetHIVSusceptibility()->GetPrognosisCompletedFraction() 
            << "," << hiv_individual->GetHIVInfection()->GetPrognosis() 
            << "," << hiv_individual->GetHIVInfection()->GetStage() 
            << "," << hiv_individual->GetHIVInfection()->GetViralLoad() 
            << "," << hiv_individual->GetHIVInfection()->GetWHOStage()

            << "," << individual->GetInfectiousness()
            << "," << individual->GetSusceptibilityContext()->getModAcquire()
            << "," << individual->GetSusceptibilityContext()->getModTransmit()
            << "," << individual->GetSusceptibilityContext()->getModMortality()

            << "," << hiv_individual->GetHIVInterventionsContainer()->GetArtStatus()
            << "," << hiv_individual->GetHIVInterventionsContainer()->GetInfectivitySuppression()
            << "," << hiv_individual->GetHIVInterventionsContainer()->GetDurationSinceLastStartingART()
            << "," << hiv_individual->GetHIVInterventionsContainer()->GetProbMaternalTransmissionModifier()
            << "," << hiv_individual->GetHIVInterventionsContainer()->OnArtQuery()
            
            << "," << sti_individual->GetCoInfectiveTransmissionFactor()
            << "," << sti_individual->GetCoInfectiveAcquisitionFactor()
            << "," << sti_individual->GetDebutAge()
            << "," << sti_individual->IsCircumcised()
            
            << "," << idvie->GetInterventionReducedAcquire()
            << "," << idvie->GetInterventionReducedTransmit()
            << "," << idvie->GetInterventionReducedMortality()

            << endl;
    }
}

