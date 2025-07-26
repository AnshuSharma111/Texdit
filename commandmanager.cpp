#include "commandmanager.h"
#include "servermanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QDateTime>
#include <algorithm>

CommandManager::CommandManager(ServerManager* serverManager, QObject *parent)
    : QObject(parent)
    , server(serverManager)
{
    initializeCommands();
    
    // Connect to server status changes
    connect(server, &ServerManager::statusChanged, 
            this, &CommandManager::handleServerStatusChange);
    
    // Update available commands based on current server status
    handleServerStatusChange();
    
    qDebug() << "CommandManager: Initialized with" << commands.size() << "commands";
}

void CommandManager::initializeCommands()
{
    // Define all available commands with their properties
    commands.clear();
    
    commands["summarise"] = {
        "summarise",
        "Generate a summary of the input text",
        true,  // requires server
        true   // requires input
    };
    
    commands["tone"] = {
        "tone",
        "Analyze and adjust the tone of the text",
        true,  // requires server
        true   // requires input
    };
    
    commands["keywords"] = {
        "keywords",
        "Extract key words and phrases from the text",
        true,  // requires server
        true   // requires input
    };
    
    commands["rephrase"] = {
        "rephrase",
        "Rephrase the text while maintaining meaning",
        true,  // requires server
        true   // requires input
    };
    
    commands["rewrite"] = {
        "rewrite",
        "Rewrite the text with improved clarity and structure",
        true,  // requires server
        true   // requires input
    };
    
    // Add local commands that don't require server
    commands["help"] = {
        "help",
        "Show available commands and their descriptions",
        false, // doesn't require server
        false  // doesn't require input
    };
    
    commands["clear"] = {
        "clear",
        "Clear the input text",
        false, // doesn't require server
        false  // doesn't require input
    };
}

void CommandManager::handleServerStatusChange()
{
    availableCommands.clear();
    
    bool serverReady = server->isReady();
    
    for (auto it = commands.begin(); it != commands.end(); ++it) {
        const CommandInfo& info = it.value();
        
        // Add command if it doesn't require server, or if server is ready
        if (!info.requiresServer || serverReady) {
            availableCommands.append(it.key());
        }
    }
    
    qDebug() << "CommandManager: Available commands updated:" << availableCommands.size() 
             << "of" << commands.size() << "(server ready:" << serverReady << ")";
}

QStringList CommandManager::getAllCommands() const
{
    return commands.keys();
}

QStringList CommandManager::getValidCommands() const
{
    return availableCommands;
}

CommandManager::CommandInfo CommandManager::getCommandInfo(const QString& command) const
{
    return commands.value(command, {"", "", false, false});
}

bool CommandManager::isCommandValid(const QString& command) const
{
    return commands.contains(command);
}

void CommandManager::executeCommand(const QString& command, const QString& inputText,
                                   std::function<void(CommandResult, const QString&)> callback)
{
    qDebug() << "CommandManager: Executing command:" << command;
    
    // Validate command exists
    if (!isCommandValid(command)) {
        QString error = QString("Unknown command: %1").arg(command);
        qDebug() << "CommandManager: ❌" << error;
        if (callback) callback(InvalidCommand, error);
        emit commandExecuted(command, InvalidCommand, error);
        return;
    }
    
    CommandInfo info = getCommandInfo(command);
    
    // Check if command is currently available
    if (!availableCommands.contains(command)) {
        QString error = QString("Command '%1' is not available (server required but not ready)").arg(command);
        qDebug() << "CommandManager: ❌" << error;
        if (callback) callback(ServerError, error);
        emit commandExecuted(command, ServerError, error);
        return;
    }
    
    // Validate input requirements
    if (info.requiresInput && inputText.trimmed().isEmpty()) {
        QString error = QString("Command '%1' requires input text").arg(command);
        qDebug() << "CommandManager: ❌" << error;
        if (callback) callback(ValidationError, error);
        emit commandExecuted(command, ValidationError, error);
        return;
    }
    
    // Route to appropriate execution method
    if (info.requiresServer) {
        executeServerCommand(command, inputText, callback);
    } else {
        executeLocalCommand(command, inputText, callback);
    }
}

void CommandManager::executeLocalCommand(const QString& command, const QString& inputText,
                                       std::function<void(CommandResult, const QString&)> callback)
{
    qDebug() << "CommandManager: Executing local command:" << command;
    
    QString result;
    
    if (command == "help") {
        QStringList helpText;
        helpText << "Available Commands:";
        helpText << "";
        
        for (const QString& cmd : availableCommands) {
            CommandInfo info = getCommandInfo(cmd);
            QString status = availableCommands.contains(cmd) ? "✅" : "❌";
            helpText << QString("%1 %2 - %3").arg(status, cmd, info.description);
        }
        
        result = helpText.join("\n");
        
    } else if (command == "clear") {
        result = "Input cleared";
        // Note: The actual clearing would be handled by the UI
        
    } else {
        result = QString("Local command '%1' executed successfully").arg(command);
    }
    
    qDebug() << "CommandManager: ✅ Local command completed:" << command;
    if (callback) callback(Success, result);
    emit commandExecuted(command, Success, result);
}

void CommandManager::executeServerCommand(const QString& command, const QString& inputText,
                                        std::function<void(CommandResult, const QString&)> callback)
{
    qDebug() << "CommandManager: Executing server command:" << command;
    
    // Prepare request data
    QJsonObject requestData;
    requestData["command"] = command;
    requestData["text"] = inputText;
    requestData["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    // Make server request
    server->makeRequest(
        QString("/api/%1").arg(command),
        requestData,
        [=](const QJsonObject& response) {
            // Success callback
            QString result = response.value("result").toString();
            if (result.isEmpty()) {
                result = response.value("output").toString();
            }
            if (result.isEmpty()) {
                result = QString("Command '%1' executed successfully").arg(command);
            }
            
            qDebug() << "CommandManager: ✅ Server command completed:" << command;
            if (callback) callback(Success, result);
            emit commandExecuted(command, Success, result);
        },
        [=](const QString& error) {
            // Error callback
            QString errorMsg = QString("Server command failed: %1").arg(error);
            qDebug() << "CommandManager: ❌" << errorMsg;
            if (callback) callback(ServerError, errorMsg);
            emit commandExecuted(command, ServerError, errorMsg);
        }
    );
}

void CommandManager::getSuggestions(const QString& query,
                                   std::function<void(const QStringList&)> callback)
{
    QStringList suggestions;
    QString lowerQuery = query.toLower();
    
    // Search through ALL commands, not just available ones
    for (auto it = commands.begin(); it != commands.end(); ++it) {
        const QString& command = it.key();
        if (command.toLower().contains(lowerQuery)) {
            suggestions.append(command);
        }
    }
    
    // Sort by relevance (exact match first, then starts with, then contains)
    std::sort(suggestions.begin(), suggestions.end(), [&](const QString& a, const QString& b) {
        bool aExact = a.toLower() == lowerQuery;
        bool bExact = b.toLower() == lowerQuery;
        if (aExact != bExact) return aExact;
        
        bool aStarts = a.toLower().startsWith(lowerQuery);
        bool bStarts = b.toLower().startsWith(lowerQuery);
        if (aStarts != bStarts) return aStarts;
        
        return a < b; // Alphabetical for same relevance
    });
    
    qDebug() << "CommandManager: Suggestions for" << query << ":" << suggestions;
    
    // Always call the callback with our suggestions
    if (callback) callback(suggestions);
    emit suggestionsAvailable(query, suggestions);
    
    // If server is ready, also try server-based fuzzy search for enhanced results
    if (server->isReady()) {
        QJsonObject requestData;
        requestData["query"] = query;
        
        QJsonArray choices;
        for (auto it = commands.begin(); it != commands.end(); ++it) {
            choices.append(it.key());
        }
        requestData["choices"] = choices;
        
        server->makeRequest(
            "/api/search",
            requestData,
            [=](const QJsonObject& response) {
                QStringList serverSuggestions;
                QJsonArray results = response["suggestions"].toArray();
                for (const QJsonValue& value : results) {
                    serverSuggestions.append(value.toString());
                }
                
                if (!serverSuggestions.isEmpty()) {
                    qDebug() << "CommandManager: Server enhanced suggestions:" << serverSuggestions;
                    // Emit server suggestions if they're different/better
                    emit suggestionsAvailable(query, serverSuggestions);
                }
            },
            [=](const QString& error) {
                qDebug() << "CommandManager: Server search failed:" << error;
                // Already provided local suggestions above
            }
        );
    }
}
