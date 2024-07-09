
#pragma once

#include <string>
#include <list>
#include <vector>

namespace Kernel
{
    struct IIndividualHuman;
    class RANDOMBASE;
}

namespace ReportUtilitiesMalaria
{
    // Smears infectiousness values using a binomial draw given 40 draws
    float BinomialInfectiousness( Kernel::RANDOMBASE * rng, float infec );

    // Converting +ve microscopic fields of view out of 200 slide views to parasite densities
    float FieldsOfViewToDensity( float positive_fields );

    // Adding uncertainty to PCR derived densities a la Walker et al. (2015)
    float NASBADensityWithUncertainty( Kernel::RANDOMBASE * rng, float true_density );

    // Evaluates polynomial of form : f(x) = p( 0 ) * x ^ (n - 1) + p( 1 ) *x ^ (n - 2)…
    // where n = the length of the vector p
    float PolyVal( const std::vector<double>& p, float x );

    void LogIndividualMalariaInfectionAssessment( Kernel::IIndividualHuman *pHuman,
                                                  const std::vector<float>& rDetectionThresholds,
                                                  std::vector<float>& rDetected,
                                                  float& rMeanParasitemia );

    void  CountPositiveSlideFields( Kernel::RANDOMBASE* rng,
                                    float density,
                                    int nfields,
                                    float uL_per_field,
                                    int& positive_fields );

    // Contain the information about infections for a particular barcode
    class BarcodeColumn
    {
    public:
        BarcodeColumn()
            : m_ColumnName()
            , m_Count( 0 )
        {
        }

        BarcodeColumn( const std::string& rColumnName )
            : m_ColumnName( rColumnName )
            , m_Count( 0 )
        {
        }

        ~BarcodeColumn() { }

        const std::string& GetColumnName() const { return m_ColumnName; }
        uint32_t           GetCount()      const { return m_Count; }
        void AddCount( uint32_t count ) { m_Count += count; }
        void ResetCount() { m_Count = 0; }

    private:
        std::string          m_ColumnName;
        uint32_t             m_Count;
    };
}