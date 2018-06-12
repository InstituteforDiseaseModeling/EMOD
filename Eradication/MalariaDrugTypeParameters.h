/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"

namespace Kernel 
{
    struct IStrainIdentity;
    struct IGenomeMarkers;

    ENUM_DEFINE(MalariaDrugType,
        ENUM_VALUE_SPEC(Artemisinin             , 1)
        ENUM_VALUE_SPEC(Chloroquine             , 2)
        ENUM_VALUE_SPEC(Quinine                 , 3)
        ENUM_VALUE_SPEC(SP                      , 4)
        ENUM_VALUE_SPEC(Primaquine              , 5)
        ENUM_VALUE_SPEC(Artemether_Lumefantrine , 6)
        ENUM_VALUE_SPEC(GenTransBlocking        , 7)
        ENUM_VALUE_SPEC(GenPreerythrocytic      , 8)
        ENUM_VALUE_SPEC(Tafenoquine             , 9))

    class DoseMap : public JsonConfigurable, public IComplexJsonConfigurable
    {
        // We need the following two lines because we inherit from JsonConfigurable
        // which provides a little more functionality than is needed by these little
        // "class types".
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            DoseMap() {}
            typedef std::map<float,float> dose_map_t;
            dose_map_t fractional_dose_by_upper_age;
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }
    };

    class GenomeMarkerModifiers : public JsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        GenomeMarkerModifiers( const std::string& rMarkerName = "", uint32_t genomeBitMask = 0 );
        virtual ~GenomeMarkerModifiers();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration * inputJson ) override;

        // other methods
        inline const std::string& GetMarkerName() const { return m_MarkerName; }
        inline float              GetC50()        const { return m_C50; }
        inline float              GetMaxKilling() const { return m_MaxKilling; }

        inline bool HasGenomeBit( uint32_t genomeBits ) const
        { 
            return ((genomeBits & m_GenomeBitMask) != 0);
        }

    private:
        std::string m_MarkerName;
        float m_C50;
        float m_MaxKilling;
        uint32_t m_GenomeBitMask;
    };

    class DrugResistantModifiers : public JsonConfigurable, public IComplexJsonConfigurable
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        DrugResistantModifiers( const IGenomeMarkers& rGenomeMarkers );
        virtual ~DrugResistantModifiers();

        // IComplexJsonConfigurable methods
        virtual bool  HasValidDefault() const override { return false; }
        virtual json::QuickBuilder GetSchema() override;
        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;

        // Other methods
        int Size() const;
        const GenomeMarkerModifiers& operator[]( int index ) const;

        float GetC50( const IStrainIdentity& rStrain ) const;
        float GetMaxKilling( const IStrainIdentity& rStrain ) const;

    private:
        std::vector<GenomeMarkerModifiers> m_ModifierCollection;
    };

    class MalariaDrugTypeParameters : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        static MalariaDrugTypeParameters* CreateMalariaDrugTypeParameters( const Configuration* inputJson, 
                                                                           const std::string& drugType,
                                                                           const IGenomeMarkers& rGenomeMarkers );

        virtual ~MalariaDrugTypeParameters();
        virtual bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        typedef map< std::string, MalariaDrugTypeParameters* > tMDTPMap;

        float GetMaxDrugIRBCKill()      const;
        float GetKillRateHepatocyte()   const;
        float GetKillRateGametocyte02() const;
        float GetKillRateGametocyte34() const;
        float GetKillRateGametocyteM()  const;
        float GetPkpdC50()              const;
        float GetCMax()                 const;
        float GetVd()                   const;
        float GetDecayT1()              const;
        float GetDecayT2()              const;
        int   GetFullTreatmentDoses()   const;
        float GetDoseInterval()         const;
        float GetBodyWeightExponent()   const;

        const DoseMap& GetDoseMap() const;
        const DrugResistantModifiers& GetResistantModifiers() const;

    protected:
        MalariaDrugTypeParameters( const std::string& drugType, const IGenomeMarkers& rGenomeMarkers );
        void Initialize(const std::string& drugType);

        float max_drug_IRBC_kill;
        float drug_hepatocyte_killrate;
        float drug_gametocyte02_killrate;
        float drug_gametocyte34_killrate;
        float drug_gametocyteM_killrate;
        float drug_pkpd_c50;
        float drug_Cmax;
        float drug_Vd;
        float drug_decay_T1;
        float drug_decay_T2;
        int   drug_fulltreatment_doses;
        float drug_dose_interval;

        float bodyweight_exponent;
        DoseMap dose_map;

        DrugResistantModifiers m_Modifiers;

    private:
        std::string _drugType;
    };
}
