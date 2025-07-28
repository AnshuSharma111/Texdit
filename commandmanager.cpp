#include "commandmanager.h"
#include "servermanager.h"
#include "commandRegistry.h"
#include <QDebug>
#include <QJsonDocument>
#include <QDateTime>
#include <algorithm>

CommandManager::CommandManager(ServerManager* serverManager, QObject *parent)
    : QObject(parent)
    , server(serverManager)
    , executionState(Idle)
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
        "Generate a summary of the input text. Usage: 'summarise' (20-30%) or 'summarise <percentage>' (e.g., 'summarise 50' for 45-55% range)",
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
    // Try direct lookup first
    if (commands.contains(command)) {
        return true;
    }
    
    // Parse command with arguments
    QString baseCommand;
    QJsonObject args;
    return parseCommandWithArgs(command, baseCommand, args) && commands.contains(baseCommand);
}

void CommandManager::executeCommand(const QString& command, const QString& inputText,
                                   std::function<void(CommandResult, const QString&)> callback)
{
    qDebug() << "CommandManager: Executing command:" << command;
    
    // Check if already executing
    if (executionState == Executing) {
        QString error = "Cannot execute command: another command is already running";
        qDebug() << "CommandManager: ❌" << error;
        if (callback) callback(ExecutionError, error);
        emit commandExecuted(command, ExecutionError, error);
        return;
    }
    
    // Set execution state
    executionState = Executing;
    emit executionStateChanged(executionState);
    
    // Parse and validate command
    QString baseCommand;
    QJsonObject args;
    
    if (!parseCommandWithArgs(command, baseCommand, args)) {
        QString error = QString("Unknown command: %1").arg(command);
        qDebug() << "CommandManager: ❌" << error;
        
        // Reset execution state
        executionState = Idle;
        emit executionStateChanged(executionState);
        
        if (callback) callback(InvalidCommand, error);
        emit commandExecuted(command, InvalidCommand, error);
        return;
    }
    
    CommandInfo info = getCommandInfo(baseCommand);
    
    // Check if base command is currently available
    if (!availableCommands.contains(baseCommand)) {
        QString error = QString("Command '%1' is not available (server required but not ready)").arg(baseCommand);
        qDebug() << "CommandManager: ❌" << error;
        
        // Reset execution state
        executionState = Idle;
        emit executionStateChanged(executionState);
        
        if (callback) callback(ServerError, error);
        emit commandExecuted(command, ServerError, error);
        return;
    }
    
    // Validate input requirements
    if (info.requiresInput && inputText.trimmed().isEmpty()) {
        QString error = QString("Command '%1' requires input text").arg(baseCommand);
        qDebug() << "CommandManager: ❌" << error;
        
        // Reset execution state
        executionState = Idle;
        emit executionStateChanged(executionState);
        
        if (callback) callback(ValidationError, error);
        emit commandExecuted(command, ValidationError, error);
        return;
    }
    
    // Create a wrapper callback that resets execution state
    auto wrappedCallback = [this, callback](CommandResult result, const QString& output) {
        // Reset execution state when command completes
        executionState = Idle;
        emit executionStateChanged(executionState);
        
        if (callback) callback(result, output);
    };
    
    // Route to appropriate execution method
    if (info.requiresServer) {
        executeServerCommand(command, inputText, wrappedCallback);
    } else {
        executeLocalCommand(baseCommand, inputText, wrappedCallback);
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
    
    // Parse command and arguments
    QString baseCommand;
    QJsonObject requestData;
    
    if (!parseCommandWithArgs(command, baseCommand, requestData)) {
        QString error = QString("Invalid command format: %1").arg(command);
        qDebug() << "CommandManager: ❌" << error;
        if (callback) callback(ValidationError, error);
        emit commandExecuted(command, ValidationError, error);
        return;
    }
    
    // Add common fields
    requestData["text"] = inputText;
    requestData["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    // Make server request
    server->makeRequest(
        QString("/api/%1").arg(baseCommand),
        requestData,
        [=](const QJsonObject& response) {
            // Success callback - format response appropriately
            QString result = formatServerResponse(baseCommand, response);
            
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

bool CommandManager::parseCommandWithArgs(const QString& command, QString& baseCommand, QJsonObject& args) const
{
    QStringList parts = command.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return false;
    }
    
    baseCommand = parts[0].toLower();
    
    // Handle specific command patterns
    if (baseCommand == "summarise" || baseCommand == "summarize") {
        baseCommand = "summarise"; // Normalize to backend spelling
        
        if (parts.size() > 1) {
            // Validate format: should be exactly "summarise <percentage>"
            if (parts.size() > 2) {
                qDebug() << "CommandManager: Too many arguments for summarise. Usage: 'summarise' or 'summarise <percentage>'";
                return false;
            }
            
            // Parse "summarise <int>" format
            bool ok;
            int percentage = parts[1].toInt(&ok);
            
            if (!ok || percentage <= 0 || percentage >= 100) {
                qDebug() << "CommandManager: Invalid summarise percentage:" << parts[1] << ". Must be between 1-99.";
                return false;
            }
            
            // Convert percentage to ratio (backend expects 0-1)
            double ratio = percentage / 100.0;
            args["ratio"] = ratio;
            
            // Calculate intelligent min/max range for better quality
            // For user-specified percentages, use a tighter range around the target
            double minRatio = qMax(0.05, ratio * 0.9);  // 90% of target, minimum 5%
            double maxRatio = ratio * 1.1;              // 110% of target
            
            args["min_ratio"] = minRatio;
            args["max_ratio"] = maxRatio;
            
            qDebug() << "CommandManager: Parsed summarise command with" << percentage 
                     << "% target ratio (" << ratio << "), range:" << (minRatio * 100) 
                     << "% -" << (maxRatio * 100) << "%";
        } else {
            // Use default ratio (25%) with a reasonable range
            args["ratio"] = 0.25;
            args["min_ratio"] = 0.20;  // 20%
            args["max_ratio"] = 0.30;  // 30%
            qDebug() << "CommandManager: Using default summarise ratio (25%) with range 20%-30%";
        }
        
        return true;
    }
    
    // For other commands, just validate they exist
    if (commands.contains(baseCommand)) {
        return true;
    }
    
    qDebug() << "CommandManager: Unknown base command:" << baseCommand;
    return false;
}

QString CommandManager::formatServerResponse(const QString& command, const QJsonObject& response) const
{
    if (command == "summarise") {
        // Handle summarise-specific response format
        if (response.contains("error")) {
            return QString("Error: %1").arg(response["error"].toString());
        }
        
        QString summary = response["summary"].toString();
        int originalLength = response["original_length"].toInt();
        int summaryLength = response["summary_length"].toInt();
        double compressionRatio = response["compression_ratio"].toDouble();
        
        // Extract performance data if available
        QString performanceInfo;
        if (response.contains("performance")) {
            QJsonObject perf = response["performance"].toObject();
            double totalTime = perf["total_time"].toDouble();
            double tokenizationTime = perf["tokenization_time"].toDouble();
            double generationTime = perf["generation_time"].toDouble();
            double decodingTime = perf["decoding_time"].toDouble();
            
            performanceInfo = QString("\n\n⚡ Performance Metrics (DistilBART-CNN-12-6):\n");
            performanceInfo += QString("• Total time: %1s\n").arg(QString::number(totalTime, 'f', 2));
            performanceInfo += QString("• Tokenization: %1s\n").arg(QString::number(tokenizationTime, 'f', 2));
            performanceInfo += QString("• Generation: %1s\n").arg(QString::number(generationTime, 'f', 2));
            performanceInfo += QString("• Decoding: %1s").arg(QString::number(decodingTime, 'f', 2));
        }
        
        QString result = summary;
        result += QString("\n\n📊 Summary Stats:\n");
        result += QString("• Original: %1 words\n").arg(originalLength);
        result += QString("• Summary: %1 words (%2%)\n").arg(summaryLength).arg(QString::number(compressionRatio * 100, 'f', 1));
        result += QString("• Quality: High-precision summary with intelligent length control");
        result += performanceInfo;
        
        return result;
    }
    
    // Default handling for other commands
    QString result = response.value("result").toString();
    if (result.isEmpty()) {
        result = response.value("output").toString();
    }
    if (result.isEmpty()) {
        result = QString("Command '%1' executed successfully").arg(command);
    }
    
    return result;
}

void CommandManager::getSuggestions(const QString& query,
                                   std::function<void(const QStringList&)> callback)
{
    // Use the new contextual suggestion system
    QStringList suggestions = commandRegistry::getContextualSuggestions(query);
    
    qDebug() << "CommandManager: Contextual suggestions for" << query << ":" << suggestions;
    
    // Always call the callback with our suggestions
    if (callback) callback(suggestions);
    emit suggestionsAvailable(query, suggestions);
    
    // If server is ready and we're looking for command matches (not arguments), 
    // also try server-based fuzzy search for enhanced results
    QString trimmedQuery = query.trimmed();
    QStringList parts = trimmedQuery.split(' ', Qt::SkipEmptyParts);
    
    if (server->isReady() && parts.size() == 1 && !trimmedQuery.isEmpty()) {
        QJsonObject requestData;
        requestData["query"] = query;
        
        QJsonArray choices;
        QStringList allCommands = commandRegistry::getAllCommands();
        for (const QString& cmd : allCommands) {
            choices.append(cmd);
        }
        requestData["choices"] = choices;
        
        server->makeRequest(
            "/api/search",
            requestData,
            [=](const QJsonObject& response) {
                QStringList serverSuggestions;
                QJsonArray results = response["results"].toArray();
                for (const QJsonValue& value : results) {
                    serverSuggestions.append(value.toString());
                }
                
                if (!serverSuggestions.isEmpty() && serverSuggestions != suggestions) {
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
