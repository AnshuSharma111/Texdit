#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

class ServerManager;

class CommandManager : public QObject
{
    Q_OBJECT

public:
    enum CommandResult {
        Success,
        InvalidCommand,
        ServerError,
        ValidationError,
        ExecutionError
    };

    enum ExecutionState {
        Idle,
        Executing
    };

    struct CommandInfo {
        QString name;
        QString description;
        bool requiresServer;
        bool requiresInput;
    };

    explicit CommandManager(ServerManager* serverManager, QObject *parent = nullptr);

    // Command registry
    QStringList getAllCommands() const;
    QStringList getValidCommands() const; // Only commands that can currently run
    CommandInfo getCommandInfo(const QString& command) const;
    bool isCommandValid(const QString& command) const;
    
    // Execution state
    ExecutionState getExecutionState() const { return executionState; }
    bool isExecuting() const { return executionState == Executing; }
    
    // Command execution
    void executeCommand(const QString& command, const QString& inputText = "",
                       std::function<void(CommandResult, const QString&)> callback = nullptr);
    
    // Suggestions
    void getSuggestions(const QString& query,
                       std::function<void(const QStringList&)> callback);

signals:
    void commandExecuted(const QString& command, CommandResult result, const QString& output);
    void executionStateChanged(ExecutionState state);
    void suggestionsAvailable(const QString& query, const QStringList& suggestions);

private slots:
    void handleServerStatusChange();

private:
    void initializeCommands();
    void executeLocalCommand(const QString& command, const QString& inputText,
                           std::function<void(CommandResult, const QString&)> callback);
    void executeServerCommand(const QString& command, const QString& inputText,
                            std::function<void(CommandResult, const QString&)> callback);
    
    // Command parsing helpers
    bool parseCommandWithArgs(const QString& command, QString& baseCommand, QJsonObject& args) const;
    QString formatServerResponse(const QString& command, const QJsonObject& response) const;
    
    ServerManager* server;
    QMap<QString, CommandInfo> commands;
    QStringList availableCommands;
    ExecutionState executionState;
};

#endif // COMMANDMANAGER_H
