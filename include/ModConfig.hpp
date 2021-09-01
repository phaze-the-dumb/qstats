#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig,

    CONFIG_VALUE(BadCuts, int, "BadCuts", 0);
    CONFIG_VALUE(BlocksHit, int, "BlocksHit", 0);
    CONFIG_VALUE(BlocksMissed, int, "BlocksMissed", 0);
    CONFIG_VALUE(LevelsPlayed, int, "LevelsPlayed", 0);
    CONFIG_VALUE(TotalScore, int, "TotalScore", 0);
    
    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(BadCuts);
        CONFIG_INIT_VALUE(BlocksHit);
        CONFIG_INIT_VALUE(BlocksMissed);
        CONFIG_INIT_VALUE(LevelsPlayed);
        CONFIG_INIT_VALUE(TotalScore);
    )
)