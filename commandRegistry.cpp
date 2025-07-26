#include "commandRegistry.h"

QStringList commandRegistry::getAllCommands () {
    return {
        "summarise",
        "tone",
        "keywords",
        "rephrase",
        "rewrite"
    };
}
