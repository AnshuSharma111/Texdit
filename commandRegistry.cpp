#include "commandRegistry.h"

QStringList commandRegistry::getAllCommands () {
    return {
        "/summarise",
        "summarise <percent>",
        "/tone formal",
        "/tone casual",
        "/keywords",
        "/rephrase",
        "/rewrite"
    };
}
