#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QAction>
#include <QKeySequence>
#include <QClipboard>
#include <QGuiApplication>
#include <QTextCursor>
#include <QListView>
#include <QStringListModel>
#include <QKeyEvent>
#include <QEvent>
#include <QTimer>
#include <QLabel>
#include <QTabWidget>
#include <QTextBrowser>
#include <QDateTime>
#include "commandmanager.h"

// Forward declarations for our managers
class ServerManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // UI Components
    QTabWidget* tabWidget;
    QTextEdit* input;
    QTextBrowser* debugLog;
    QLineEdit* command;
    QPushButton* executeButton;
    QLabel* statusLabel;
    QAction* goToCommandBox;
    QAction* toggleDebugTab;
    QVBoxLayout* layout;
    
    // Suggestion system
    QListView* suggestions;
    QStringListModel* suggestions_popup;
    
    // Managers
    ServerManager* serverManager;
    CommandManager* commandManager;
    
    // UI State
    bool suggestionsVisible;
    bool commandExecuting;
    bool debugTabVisible;
    QTimer* workingAnimationTimer;
    int workingAnimationState;
    QDateTime commandStartTime;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // UI Events
    void onPressCtrlSlash();
    void toggleDebugPanel();
    void commandTextEdited();
    void onSuggestionClicked(const QModelIndex &index);
    
    // Command System
    void executeCommand();
    void onCommandExecuted(const QString& command, int result, const QString& output);
    void onSuggestionsReceived(const QString& query, const QStringList& suggestions);
    void onCommandExecutionStateChanged(CommandManager::ExecutionState state);
    void updateWorkingAnimation();
    
    // Server Status
    void onServerStatusChanged(int status);

private:
    // UI Helpers
    void setupUI();
    void setupConnections();
    void setupSuggestions();
    void logDebugEvent(const QString& message);
    void displaySuggestions(const QStringList& suggestionList);
    void hideSuggestions();
    void updateServerStatus(const QString& message, bool isError = false);
    void showCommandFeedback(const QString& commandName, bool success, const QString& message);
    
    // Input handling
    void clearCommand();
    void selectSuggestion(int index);
};

#endif // MAINWINDOW_H
