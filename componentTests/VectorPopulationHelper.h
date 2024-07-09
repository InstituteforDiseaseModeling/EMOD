
#pragma once

#include "VectorCohortCollection.h"

void CheckQueueInitialization( bool isFemale,
                               uint32_t expectedNumPop,
                               uint32_t daysBetweenFeeds,
                               const Kernel::VectorCohortCollectionAbstract& rQueue );
