
#include "HIVEnums.h"

#define NUM_WHO_STAGES (4)
#define MIN_WHO_HIV_STAGE (1) // sometimes 1 is a magic number
#define MAX_WHO_HIV_STAGE (NUM_WHO_STAGES+1)

namespace Kernel
{
    class IInfectionHIV : public ISupports
    {
    public:
        virtual float GetWHOStage() const = 0;
        virtual NaturalNumber GetViralLoad() const = 0;
        virtual float GetPrognosis() const = 0;
        virtual float GetTimeInfected() const = 0;
        virtual float GetDaysTillDeath() const = 0;
        virtual const HIVInfectionStage::Enum& GetStage() const = 0;
        virtual void SetupSuppressedDiseaseTimers( float durationFromEnrollmentToArtAidsDeath ) = 0;
        virtual void ApplySuppressionDropout() = 0;
        virtual void ApplySuppressionFailure() = 0;
    };
}
