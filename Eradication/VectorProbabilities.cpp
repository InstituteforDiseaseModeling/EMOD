/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "VectorProbabilities.h"
#include "Common.h"

namespace Kernel
{
    void VectorProbabilities::serialize(IArchive& ar, VectorProbabilities*& probabilities)
    {
        probabilities = ar.IsWriter() ? probabilities : new VectorProbabilities();

        ar.startObject();
            ar.labelElement("effective_host_population") & probabilities->effective_host_population;

            ar.labelElement("outdoorareakilling") & probabilities->outdoorareakilling;
            ar.labelElement("outdoorareakilling_male") & probabilities->outdoorareakilling_male;
            ar.labelElement("diebeforeattempttohumanfeed") & probabilities->diebeforeattempttohumanfeed;
            ar.labelElement("diewithoutattemptingfeed") & probabilities->diewithoutattemptingfeed;
            ar.labelElement("survivewithoutsuccessfulfeed") & probabilities->survivewithoutsuccessfulfeed;
            ar.labelElement("successfulfeed_animal") & probabilities->successfulfeed_animal;
            ar.labelElement("successfulfeed_AD") & probabilities->successfulfeed_AD;
            ar.labelElement("indoorattempttohumanfeed") & probabilities->indoorattempttohumanfeed;
            ar.labelElement("outdoorattempttohumanfeed") & probabilities->outdoorattempttohumanfeed;

            ar.labelElement("outdoor_returningmortality") & probabilities->outdoor_returningmortality;

            ar.labelElement("indoor_diebeforefeeding") & probabilities->indoor_diebeforefeeding;
            ar.labelElement("indoor_hostnotavailable") & probabilities->indoor_hostnotavailable;
            ar.labelElement("indoor_dieduringfeeding") & probabilities->indoor_dieduringfeeding;
            ar.labelElement("indoor_diepostfeeding") & probabilities->indoor_diepostfeeding;
            ar.labelElement("indoor_successfulfeed_human") & probabilities->indoor_successfulfeed_human;
            ar.labelElement("indoor_successfulfeed_AD") & probabilities->indoor_successfulfeed_AD;

            ar.labelElement("outdoor_diebeforefeeding") & probabilities->outdoor_diebeforefeeding;
            ar.labelElement("outdoor_hostnotavailable") & probabilities->outdoor_hostnotavailable;
            ar.labelElement("outdoor_dieduringfeeding") & probabilities->outdoor_dieduringfeeding;
            ar.labelElement("outdoor_diepostfeeding") & probabilities->outdoor_diepostfeeding;
            ar.labelElement("outdoor_successfulfeed_human") & probabilities->outdoor_successfulfeed_human;

            ar.labelElement("sugarTrapKilling") & probabilities->sugarTrapKilling;
            ar.labelElement("individualRepellentBlock") & probabilities->individualRepellentBlock;

            ar.labelElement("attraction_ADOV") & probabilities->attraction_ADOV;
            ar.labelElement("attraction_ADIV") & probabilities->attraction_ADIV;
            ar.labelElement("kill_livestockfeed") & probabilities->kill_livestockfeed;
            ar.labelElement("kill_PFV") & probabilities->kill_PFV;
            ar.labelElement("spatial_repellent") & probabilities->spatial_repellent;
            ar.labelElement("nooutdoorhumanfound") & probabilities->nooutdoorhumanfound;
            ar.labelElement("outdoorRestKilling") & probabilities->outdoorRestKilling;
        ar.endObject();
    }

    VectorProbabilities::VectorProbabilities() : 
        effective_host_population(0.0f),
        outdoorareakilling(0.0f),
        outdoorareakilling_male(0.0f),
        diebeforeattempttohumanfeed(0.0f),
        diewithoutattemptingfeed(0.0f),
        survivewithoutsuccessfulfeed(0.0f),
        successfulfeed_animal(0.0f),
        successfulfeed_AD(0.0f),
        indoorattempttohumanfeed(0.0f),
        outdoorattempttohumanfeed(0.0f),
        outdoor_returningmortality(0.0f),
        indoor_diebeforefeeding(0.0f),
        indoor_hostnotavailable(0.0f),
        indoor_dieduringfeeding(0.0f),
        indoor_diepostfeeding(0.0f),
        indoor_successfulfeed_human(0.0f),
        indoor_successfulfeed_AD(0.0f),
        outdoor_diebeforefeeding(0.0f),
        outdoor_hostnotavailable(0.0f),
        outdoor_dieduringfeeding(0.0f),
        outdoor_diepostfeeding(0.0f),
        outdoor_successfulfeed_human(0.0f),
        sugarTrapKilling(0.0f),
        individualRepellentBlock(0.0f),
        attraction_ADOV(0.0f),
        attraction_ADIV(0.0f),
        kill_livestockfeed(0.0f),
        kill_PFV(0.0f),
        spatial_repellent(0.0f),
        nooutdoorhumanfound(0.0f),
        outdoorRestKilling(0.0f)
    {
    }

    VectorProbabilities::~VectorProbabilities()
    {
    }

    VectorProbabilities* VectorProbabilities::CreateVectorProbabilities()
    {
        VectorProbabilities* probs = _new_ VectorProbabilities();
        return probs;
    }

    void VectorProbabilities::ResetProbabilities()
    {
        // vector-feeding and MC weighted population
        effective_host_population = 0;

        // probabilities from community-level interventions
        outdoorareakilling = 0;
        outdoorareakilling_male = 0;
        attraction_ADOV = 0;
        kill_PFV = 0;
        attraction_ADIV = 0;
        kill_livestockfeed = 0;
        spatial_repellent = 0;
        nooutdoorhumanfound = 0;

        outdoorRestKilling = 0;

        // reset indoor probabilities
        indoor_diebeforefeeding = 0;
        indoor_hostnotavailable = 0;
        indoor_dieduringfeeding = 0;
        indoor_diepostfeeding = 0;
        indoor_successfulfeed_human = 0;
        indoor_successfulfeed_AD = 0;

        // reset outdoor probabilities
        outdoor_diebeforefeeding = 0;
        outdoor_hostnotavailable = 0;
        outdoor_dieduringfeeding = 0;
        outdoor_diepostfeeding = 0;
        outdoor_successfulfeed_human = 0;

        // reset lifecycle killing probabilities
        sugarTrapKilling = 0;
        individualRepellentBlock = 0;
    }

    void VectorProbabilities::AccumulateIndividualProbabilities(IVectorInterventionsEffects* ivie, float host_vector_weight)
    {
        indoor_diebeforefeeding       +=  host_vector_weight * ivie->GetDieBeforeFeeding();           // die before feeding
        indoor_hostnotavailable       +=  host_vector_weight * ivie->GetHostNotAvailable();           // host not available, but vector survives
        indoor_dieduringfeeding       +=  host_vector_weight * ivie->GetDieDuringFeeding();           // die during feeding
        indoor_diepostfeeding         +=  host_vector_weight * ivie->GetDiePostFeeding();             // die post feeding
        indoor_successfulfeed_human   +=  host_vector_weight * ivie->GetSuccessfulFeedHuman();        // successful feed_human
        indoor_successfulfeed_AD      +=  host_vector_weight * ivie->GetSuccessfulFeedAD();           // successful feed_AD
 
        outdoor_diebeforefeeding      +=  host_vector_weight * ivie->GetOutdoorDieBeforeFeeding();    // die before feeding on outdoor feeds
        outdoor_hostnotavailable      +=  host_vector_weight * ivie->GetOutdoorHostNotAvailable();    // host not available, but vector survives
        outdoor_dieduringfeeding      +=  host_vector_weight * ivie->GetOutdoorDieDuringFeeding();    // die during feeding
        outdoor_diepostfeeding        +=  host_vector_weight * ivie->GetOutdoorDiePostFeeding();      // die post feeding
        outdoor_successfulfeed_human  +=  host_vector_weight * ivie->GetOutdoorSuccessfulFeedHuman(); // successful feed_human

        effective_host_population     += host_vector_weight;
    }

    void VectorProbabilities::NormalizeIndividualProbabilities()
    {
        if(effective_host_population > 0)
        {
            indoor_diebeforefeeding      /=  effective_host_population;
            indoor_hostnotavailable      /=  effective_host_population;
            indoor_dieduringfeeding      /=  effective_host_population;
            indoor_diepostfeeding        /=  effective_host_population;
            indoor_successfulfeed_human  /=  effective_host_population;
            indoor_successfulfeed_AD     /=  effective_host_population;

            outdoor_diebeforefeeding     /=  effective_host_population;
            outdoor_hostnotavailable     /=  effective_host_population;
            outdoor_dieduringfeeding     /=  effective_host_population;
            outdoor_diepostfeeding       /=  effective_host_population;
            outdoor_successfulfeed_human /=  effective_host_population;
        }
        else
        {
            indoor_hostnotavailable = 1;
        }
    }

    void VectorProbabilities::SetNodeProbabilities(INodeVectorInterventionEffects* invie, float dt)
    {
        outdoorareakilling      = EXPCDF( -dt * invie->GetOutdoorKilling() );
        outdoorareakilling_male = EXPCDF( -dt * invie->GetOutdoorKillingMale() );
        
        kill_PFV           = invie->GetPFVKill();
        attraction_ADIV    = invie->GetADIVAttraction();
        attraction_ADOV    = invie->GetADOVAttraction();
        spatial_repellent  = invie->GetVillageSpatialRepellent();
        sugarTrapKilling   = invie->GetSugarFeedKilling();
        kill_livestockfeed = invie->GetAnimalFeedKilling();
        outdoorRestKilling = invie->GetOutdoorRestKilling();
    }

    void VectorProbabilities::FinalizeTransitionProbabilites(float anthropophily, float indoor_feeding)
    {
        diewithoutattemptingfeed      =  outdoorareakilling; // for those that do not attempt to feed
        survivewithoutsuccessfulfeed  =  (1.0f - attraction_ADOV) * (1.0f - kill_PFV) * (spatial_repellent * (1.0f - outdoorareakilling) + (1.0f - spatial_repellent) * (1.0f - attraction_ADIV) * anthropophily * (1.0f - indoor_feeding) * (1.0f - outdoorareakilling) * nooutdoorhumanfound);
        successfulfeed_animal         =  (1.0f - attraction_ADOV) * (1.0f - kill_PFV) * (1.0f - spatial_repellent) * (1.0f - attraction_ADIV) * (1.0f - anthropophily) * (1.0f - outdoorareakilling) * (1.0f - kill_livestockfeed) * (1.0f - kill_PFV);
        successfulfeed_AD             =  (attraction_ADOV * (1.0f - outdoorareakilling) + (1.0f - attraction_ADOV) * (1.0f - kill_PFV) * (1.0f - spatial_repellent) * attraction_ADIV * (1.0f - outdoorareakilling) * (1.0f - kill_PFV));
        indoorattempttohumanfeed      =  (1.0f - attraction_ADOV) * (1.0f - kill_PFV) * (1.0f - spatial_repellent) * (1.0f - attraction_ADIV) * anthropophily * indoor_feeding;
        outdoorattempttohumanfeed     =  (1.0f - attraction_ADOV) * (1.0f - kill_PFV) * (1.0f - spatial_repellent) * (1.0f - attraction_ADIV) * anthropophily * (1.0f - indoor_feeding) * (1.0f - outdoorareakilling) * (1.0f - nooutdoorhumanfound);
        diebeforeattempttohumanfeed   =  1.0f - (survivewithoutsuccessfulfeed + successfulfeed_animal + successfulfeed_AD + indoorattempttohumanfeed + outdoorattempttohumanfeed);

        // outdoor probabilities
        outdoor_returningmortality = outdoorRestKilling + (1.0f - outdoorRestKilling) * kill_PFV;

        // correct indoor probabilities for presence of PF-V, if applicable
        if (kill_PFV > 0)
        { 
            indoor_diebeforefeeding      +=  indoor_successfulfeed_AD * kill_PFV;
            indoor_diepostfeeding        +=  indoor_successfulfeed_human * kill_PFV;
            indoor_successfulfeed_human  *=  1.0f - kill_PFV;
            indoor_successfulfeed_AD     *=  1.0f - kill_PFV;
        }
    }
}
