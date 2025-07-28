#include "commandRegistry.h"
#include <QDebug>

QStringList commandRegistry::getAllCommands () {
    return {
        "summarise",
        "tone",
        "font",
        "highlight",
        "keywords",
        "rephrase",
        "rewrite"
    };
}

QMap<QString, CommandInfo> commandRegistry::getCommandDefinitions() {
    static QMap<QString, CommandInfo> commands = {
        {
            "summarise", 
            {
                "summarise",
                "Summarize text with specified compression ratio",
                {"<ratio>"},
                "summarise <ratio> - ratio between 0.1 and 1.0 (e.g., summarise 0.3)"
            }
        },
        {
            "tone", 
            {
                "tone",
                "Change or analyze text tone",
                {"formal", "casual", "playful"},
                "tone <style> - changes text tone (formal, casual, playful)"
            }
        },
        {
            "font", 
            {
                "font",
                "Change font family of selected text",
                {"<font-name>"},
                "font <font-name> - changes font (e.g., font Arial, font Times)"
            }
        },
        {
            "highlight", 
            {
                "highlight",
                "Highlight specific elements in text",
                {"keywords", "grammar"},
                "highlight <type> - highlights keywords or grammar issues"
            }
        },
        {
            "keywords", 
            {
                "keywords",
                "Extract key words and phrases from text",
                {},
                "keywords - extracts important keywords from selected text"
            }
        },
        {
            "rephrase", 
            {
                "rephrase",
                "Rephrase text in different words",
                {},
                "rephrase - rewrites selected text with different phrasing"
            }
        },
        {
            "rewrite", 
            {
                "rewrite",
                "Completely rewrite text with improved style",
                {},
                "rewrite - completely rewrites selected text for better clarity"
            }
        }
    };
    return commands;
}

QStringList commandRegistry::getCommandArguments(const QString& command) {
    QMap<QString, CommandInfo> definitions = getCommandDefinitions();
    if (definitions.contains(command)) {
        return definitions[command].arguments;
    }
    return {};
}

QString commandRegistry::getCommandUsage(const QString& command) {
    QMap<QString, CommandInfo> definitions = getCommandDefinitions();
    if (definitions.contains(command)) {
        return definitions[command].usage;
    }
    return "";
}

QStringList commandRegistry::getContextualSuggestions(const QString& input) {
    QString trimmedInput = input.trimmed();
    QStringList suggestions;
    
    qDebug() << "CommandRegistry: Getting suggestions for input:" << trimmedInput;
    
    if (trimmedInput.isEmpty()) {
        // Show only a few starter commands when empty
        suggestions << "summarise" << "tone" << "highlight";
        qDebug() << "CommandRegistry: Empty input, showing starter commands:" << suggestions;
        return suggestions;
    }
    
    // Split input to analyze command vs arguments
    QStringList parts = trimmedInput.split(' ', Qt::KeepEmptyParts);
    qDebug() << "CommandRegistry: Input parts:" << parts << "Count:" << parts.size();
    
    if (parts.size() == 1) {
        // User is typing a command name - show ONLY matching commands (like Minecraft)
        QString partial = parts[0].toLower();
        QStringList allCommands = getAllCommands();
        
        // Find all matching commands but keep them minimal
        for (const QString& cmd : allCommands) {
            if (cmd.startsWith(partial, Qt::CaseInsensitive)) {
                suggestions << cmd;
            }
        }
        
        // Limit to 3 suggestions max for clean UI
        if (suggestions.size() > 3) {
            suggestions = suggestions.mid(0, 3);
        }
        
        qDebug() << "CommandRegistry: Command suggestions:" << suggestions;
    } else {
        // User has typed command + space - show argument completions
        QString commandName = parts[0].toLower();
        QString currentArg = parts.size() > 1 ? parts[1] : "";
        
        qDebug() << "CommandRegistry: Command:" << commandName << "Current arg:" << currentArg;
        
        if (commandName == "summarise") {
            // Show argument options that start with current input
            QStringList options = {"10", "25", "50", "75"};
            for (const QString& option : options) {
                if (currentArg.isEmpty() || option.startsWith(currentArg)) {
                    suggestions << commandName + " " + option;
                }
            }
        } else if (commandName == "tone") {
            QStringList options = {"formal", "casual", "playful"};
            for (const QString& option : options) {
                if (currentArg.isEmpty() || option.startsWith(currentArg, Qt::CaseInsensitive)) {
                    suggestions << commandName + " " + option;
                }
            }
        } else if (commandName == "font") {
            QStringList options = {"Arial", "Calibri", "Georgia", "Verdana"};
            for (const QString& option : options) {
                if (currentArg.isEmpty() || option.startsWith(currentArg, Qt::CaseInsensitive)) {
                    suggestions << commandName + " " + option;
                }
            }
        } else if (commandName == "highlight") {
            QStringList options = {"keywords", "grammar"};
            for (const QString& option : options) {
                if (currentArg.isEmpty() || option.startsWith(currentArg, Qt::CaseInsensitive)) {
                    suggestions << commandName + " " + option;
                }
            }
        } else {
            // For commands without arguments, just suggest the command itself
            QStringList allCommands = getAllCommands();
            if (allCommands.contains(commandName)) {
                suggestions << commandName;
            }
        }
        
        qDebug() << "CommandRegistry: Argument suggestions:" << suggestions;
    }
    
    return suggestions;
}