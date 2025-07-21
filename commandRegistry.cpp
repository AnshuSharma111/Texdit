#include "commandRegistry.h"

QStringList commandRegistry::getAllCommands () {
    return {
        "/summarise",
        "/tone formal",
        "/tone casual",
        "/keywords"
    };
}
