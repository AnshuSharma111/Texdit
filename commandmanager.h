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
    
    // Command execution
    void executeCommand(const QString& command, const QString& inputText = "",
                       std::function<void(CommandResult, const QString&)> callback = nullptr);
    
    // Suggestions
    void getSuggestions(const QString& query,
                       std::function<void(const QStringList&)> callback);

signals:
    void commandExecuted(const QString& command, CommandResult result, const QString& output);
    void suggestionsAvailable(const QString& query, const QStringList& suggestions);

private slots:
    void handleServerStatusChange();

private:
    void initializeCommands();
    void executeLocalCommand(const QString& command, const QString& inputText,
                           std::function<void(CommandResult, const QString&)> callback);
    void executeServerCommand(const QString& command, const QString& inputText,
                            std::function<void(CommandResult, const QString&)> callback);
    
    ServerManager* server;
    QMap<QString, CommandInfo> commands;
    QStringList availableCommands;
};

#endif // COMMANDMANAGER_H
