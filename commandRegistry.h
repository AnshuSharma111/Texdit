#ifndef COMMANDREGISTRY_H
#define COMMANDREGISTRY_H

#include <QStringList>
#include <QMap>

struct CommandInfo {
    QString name;
    QString description;
    QStringList arguments;
    QString usage;
};

class commandRegistry {

public:
    static QStringList getAllCommands();
    static QMap<QString, CommandInfo> getCommandDefinitions();
    static QStringList getCommandArguments(const QString& command);
    static QString getCommandUsage(const QString& command);
    static QStringList getContextualSuggestions(const QString& input);

};

#endif // COMMANDREGISTRY_H
