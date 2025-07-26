#include "mainwindow.h"
#include "servermanager.h"
#include "commandmanager.h"
#include <QDebug>
#include <QClipboard>
#include <QGuiApplication>
#include <QApplication>
#include <QTimer>
#include <QDateTime>
#include <QMouseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , suggestionsVisible(false)
{
    // Initialize managers first
    serverManager = new ServerManager(this);
    commandManager = new CommandManager(serverManager, this);
    
    // Setup UI
    setupUI();
    setupConnections();
    setupSuggestions();
    
    // Start health monitoring since server should already be running
    QTimer::singleShot(1000, this, [this]() {
        if (serverManager) {
            serverManager->startHealthMonitoring();
        }
    });
    
    qDebug() << "MainWindow: Initialized with robust foundation";
}

MainWindow::~MainWindow()
{
    // Managers will be cleaned up automatically by Qt parent-child relationship
    qDebug() << "MainWindow: Destroyed";
}

void MainWindow::setupUI()
{
    // Create central widget and main layout
    QWidget *centralWidget = new QWidget(this);
    layout = new QVBoxLayout(centralWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Create main text input area
    input = new QTextEdit(this);
    input->setPlaceholderText("Enter text here...");
    
    // Set clipboard content if available
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard->text().isEmpty()) {
        input->setText(clipboard->text());
    }
    
    // Create command input area with execute button
    QHBoxLayout* commandLayout = new QHBoxLayout();
    
    command = new QLineEdit(this);
    command->setPlaceholderText("Type command... (Ctrl + /)");
    
    executeButton = new QPushButton("Execute", this);
    executeButton->setEnabled(true); // Always enabled - let validator handle filtering
    executeButton->setToolTip("Execute command");
    executeButton->setFixedWidth(80);
    
    commandLayout->addWidget(command, 1);
    commandLayout->addWidget(executeButton, 0);
    commandLayout->setSpacing(5);
    
    // Create status label
    statusLabel = new QLabel("Ready", this);
    statusLabel->setStyleSheet(
        "QLabel {"
        "    color: #7f8c8d;"
        "    font-size: 12px;"
        "    padding: 5px;"
        "}"
    );
    
    // Add widgets to main layout
    layout->addWidget(input, 8);           // Main text area (80% of space)
    layout->addLayout(commandLayout, 0);   // Command input (fixed height)
    layout->addWidget(statusLabel, 0);     // Status (fixed height)
    
    setCentralWidget(centralWidget);
    
    // Setup keyboard shortcut for command focus
    goToCommandBox = new QAction(tr("Focus Command Box"), this);
    goToCommandBox->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    addAction(goToCommandBox);
    
    qDebug() << "MainWindow: UI setup complete";
}

void MainWindow::setupConnections()
{
    // Command input connections
    connect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);
    connect(command, &QLineEdit::returnPressed, this, &MainWindow::executeCommand);
    connect(executeButton, &QPushButton::clicked, this, &MainWindow::executeCommand);
    
    // Keyboard shortcut
    connect(goToCommandBox, &QAction::triggered, this, &MainWindow::onPressCtrlSlash);
    
    // Manager connections
    connect(serverManager, &ServerManager::statusChanged, this, &MainWindow::onServerStatusChanged);
    connect(commandManager, &CommandManager::commandExecuted, this, &MainWindow::onCommandExecuted);
    connect(commandManager, &CommandManager::suggestionsAvailable, this, &MainWindow::onSuggestionsReceived);
    
    // Install event filter for advanced input handling
    command->installEventFilter(this);
    
    qDebug() << "MainWindow: Connections setup complete";
}

void MainWindow::setupSuggestions()
{
    // Create suggestions popup
    suggestions = new QListView(this);
    suggestions_popup = new QStringListModel(this);
    
    suggestions->setModel(suggestions_popup);
    suggestions->setWindowFlags(Qt::ToolTip);
    suggestions->setFocusPolicy(Qt::NoFocus);
    suggestions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    suggestions->setSelectionMode(QAbstractItemView::SingleSelection);
    suggestions->setUniformItemSizes(true);
    suggestions->setMouseTracking(true);
    suggestions->hide();
    
    // Connect suggestion selection
    connect(suggestions, &QListView::clicked, this, &MainWindow::onSuggestionClicked);
    connect(suggestions, &QListView::activated, this, &MainWindow::onSuggestionClicked);
    
    // Install event filter on suggestions for mouse handling
    suggestions->installEventFilter(this);
    
    // Hide suggestions when focus changes
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *old, QWidget *now) {
        if (suggestionsVisible && now != command) {
            hideSuggestions();
        }
    });
    
    qDebug() << "MainWindow: Suggestions setup complete";
}

void MainWindow::onPressCtrlSlash()
{
    if (!command->hasFocus()) {
        command->setFocus();
        command->selectAll(); // Select all text for easy replacement
        qDebug() << "MainWindow: Focus shifted to command input";
    }
}

void MainWindow::commandTextEdited()
{
    QString text = command->text().trimmed();
    qDebug() << "MainWindow: Command text changed to:" << text;
    
    if (text.isEmpty()) {
        hideSuggestions();
        return;
    }
    
    // Check if this is already a complete command
    if (commandManager->isCommandValid(text)) {
        qDebug() << "MainWindow: Complete command entered, hiding suggestions";
        hideSuggestions();
        return;
    }
    
    // Get suggestions for partial input
    commandManager->getSuggestions(text, [this](const QStringList& suggestions) {
        // This callback will be called when suggestions are ready
        // The onSuggestionsReceived slot will handle the actual display
    });
}

void MainWindow::executeCommand()
{
    QString commandText = command->text().trimmed();
    
    if (commandText.isEmpty()) {
        updateServerStatus("Please enter a command", true);
        return;
    }
    
    qDebug() << "MainWindow: Executing command:" << commandText;
    
    // Hide suggestions
    hideSuggestions();
    
    // Show execution feedback
    updateServerStatus(QString("Executing '%1'...").arg(commandText));
    
    // Get input text
    QString inputText = input->toPlainText();
    
    // Execute command through command manager
    commandManager->executeCommand(commandText, inputText, [this](int result, const QString& output) {
        // This callback will be called when command execution is complete
        // The onCommandExecuted slot will handle the actual result processing
    });
}

void MainWindow::onCommandExecuted(const QString& command, int result, const QString& output)
{
    CommandManager::CommandResult cmdResult = static_cast<CommandManager::CommandResult>(result);
    bool success = (cmdResult == CommandManager::Success);
    
    qDebug() << "MainWindow: Command" << command << "completed with result:" << result;
    
    showCommandFeedback(command, success, output);
    
    if (success) {
        // Handle successful command execution
        if (command == "clear") {
            input->clear();
        } else if (command == "help") {
            // Show help in a message or separate area
            input->append("\n\n--- Help ---\n" + output);
        } else {
            // For AI commands, show result
            input->append("\n\n--- " + command.toUpper() + " Result ---\n" + output);
        }
        
        // Clear command after successful execution
        QTimer::singleShot(1500, this, &MainWindow::clearCommand);
    }
}

void MainWindow::onSuggestionsReceived(const QString& query, const QStringList& suggestions)
{
    // Only show suggestions if the query still matches current input
    if (command->text().trimmed() == query && !suggestions.isEmpty()) {
        displaySuggestions(suggestions);
    }
}

void MainWindow::onServerStatusChanged(int status)
{
    ServerManager::ServerStatus serverStatus = static_cast<ServerManager::ServerStatus>(status);
    
    switch (serverStatus) {
        case ServerManager::Disconnected:
            updateServerStatus("Server disconnected", true);
            break;
        case ServerManager::Connecting:
            updateServerStatus("Connecting to server...");
            break;
        case ServerManager::Connected:
            updateServerStatus("Server connected - All features available");
            break;
        case ServerManager::Error:
            updateServerStatus("Server error - Local commands only", true);
            break;
    }
    
    qDebug() << "MainWindow: Server status changed to:" << status;
}

void MainWindow::displaySuggestions(const QStringList& suggestionList)
{
    if (suggestionList.isEmpty()) {
        hideSuggestions();
        return;
    }
    
    qDebug() << "MainWindow: Displaying" << suggestionList.size() << "suggestions";
    
    suggestions_popup->setStringList(suggestionList);
    
    // Position suggestions below command input
    int cursorPos = command->cursorPosition();
    QString textBeforeCursor = command->text().left(cursorPos);
    int textWidth = command->fontMetrics().horizontalAdvance(textBeforeCursor);
    
    QPoint caretPos(textWidth, command->height());
    QPoint globalCaretPos = command->mapToGlobal(caretPos);
    
    // Calculate popup size
    int rowCount = suggestions_popup->rowCount();
    int itemHeight = suggestions->sizeHintForRow(0);
    if (itemHeight <= 0) itemHeight = 25; // Fallback height
    
    int popupHeight = itemHeight * qMin(rowCount, 6); // Max 6 items visible
    int popupWidth = 250;
    
    suggestions->resize(popupWidth, popupHeight);
    suggestions->move(globalCaretPos);
    
    // Select first item by default
    if (rowCount > 0) {
        QModelIndex firstIndex = suggestions_popup->index(0, 0);
        suggestions->setCurrentIndex(firstIndex);
    }
    
    suggestions->show();
    suggestionsVisible = true;
    
    qDebug() << "MainWindow: Suggestions popup shown at:" << globalCaretPos;
}

void MainWindow::hideSuggestions()
{
    if (suggestionsVisible) {
        suggestions->hide();
        suggestionsVisible = false;
        qDebug() << "MainWindow: Suggestions hidden";
    }
}

void MainWindow::onSuggestionClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        qDebug() << "MainWindow: Invalid suggestion index clicked";
        return;
    }
    
    QString selectedText = index.data(Qt::DisplayRole).toString();
    qDebug() << "MainWindow: Suggestion selected:" << selectedText;
    
    selectSuggestion(index.row());
}

void MainWindow::selectSuggestion(int index)
{
    if (index < 0 || index >= suggestions_popup->rowCount()) {
        return;
    }
    
    QModelIndex modelIndex = suggestions_popup->index(index, 0);
    QString selectedText = modelIndex.data(Qt::DisplayRole).toString();
    
    // Hide suggestions first
    hideSuggestions();
    
    // Temporarily disconnect textChanged to prevent retriggering suggestions
    disconnect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);
    
    command->setText(selectedText);
    command->setCursorPosition(selectedText.length());
    
    // Reconnect after a brief delay
    QTimer::singleShot(50, this, [this]() {
        if (command) {
            connect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);
        }
    });
    
    qDebug() << "MainWindow: Selected suggestion applied:" << selectedText;
}

void MainWindow::updateServerStatus(const QString& message, bool isError)
{
    statusLabel->setText(message);
    
    if (isError) {
        statusLabel->setStyleSheet(
            "QLabel {"
            "    color: #e74c3c;"
            "    font-size: 12px;"
            "    padding: 5px;"
            "}"
        );
    } else {
        statusLabel->setStyleSheet(
            "QLabel {"
            "    color: #27ae60;"
            "    font-size: 12px;"
            "    padding: 5px;"
            "}"
        );
    }
    
    // Auto-clear status after 5 seconds for non-error messages
    if (!isError) {
        QTimer::singleShot(5000, this, [this, message]() {
            if (statusLabel && statusLabel->text() == message) { // Only clear if message hasn't changed
                statusLabel->setText("Ready");
                statusLabel->setStyleSheet(
                    "QLabel {"
                    "    color: #7f8c8d;"
                    "    font-size: 12px;"
                    "    padding: 5px;"
                    "}"
                );
            }
        });
    }
}

void MainWindow::showCommandFeedback(const QString& commandName, bool success, const QString& message)
{
    QString feedback;
    
    if (success) {
        feedback = QString("✅ '%1' executed successfully").arg(commandName);
        command->setStyleSheet(
            "QLineEdit {"
            "    border: 2px solid #27ae60;"
            "    background-color: #d5f4e6;"
            "    color: #155724;"
            "}"
        );
    } else {
        feedback = QString("❌ '%1' failed: %2").arg(commandName, message);
        command->setStyleSheet(
            "QLineEdit {"
            "    border: 2px solid #e74c3c;"
            "    background-color: #f8d7da;"
            "    color: #721c24;"
            "}"
        );
    }
    
    updateServerStatus(feedback, !success);
    
    // Reset command styling after feedback period
    QTimer::singleShot(2000, this, [this]() {
        if (command) {
            command->setStyleSheet("");
        }
    });
}

void MainWindow::clearCommand()
{
    command->clear();
    command->setStyleSheet("");
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // Handle command box key events
    if (obj == command && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        // Only handle navigation keys when suggestions are visible
        if (suggestionsVisible) {
            switch (keyEvent->key()) {
                case Qt::Key_Down: {
                    // Move down in suggestions
                    QModelIndex currentIndex = suggestions->currentIndex();
                    int nextRow = currentIndex.isValid() ? currentIndex.row() + 1 : 0;
                    int maxRow = suggestions_popup->rowCount() - 1;
                    
                    if (nextRow <= maxRow) {
                        QModelIndex nextIndex = suggestions_popup->index(nextRow, 0);
                        suggestions->setCurrentIndex(nextIndex);
                    }
                    return true; // Event handled
                }
                
                case Qt::Key_Up: {
                    // Move up in suggestions
                    QModelIndex currentIndex = suggestions->currentIndex();
                    int prevRow = currentIndex.isValid() ? currentIndex.row() - 1 : -1;
                    
                    if (prevRow >= 0) {
                        QModelIndex prevIndex = suggestions_popup->index(prevRow, 0);
                        suggestions->setCurrentIndex(prevIndex);
                    }
                    return true; // Event handled
                }
                
                case Qt::Key_Return:
                case Qt::Key_Enter: {
                    // Select current suggestion if suggestions are visible
                    QModelIndex currentIndex = suggestions->currentIndex();
                    if (currentIndex.isValid()) {
                        selectSuggestion(currentIndex.row());
                        return true; // Event handled - don't execute command
                    }
                    // If no suggestion selected, let the normal returnPressed signal handle execution
                    break;
                }
                
                case Qt::Key_Escape: {
                    // Hide suggestions on Escape
                    hideSuggestions();
                    return true; // Event handled
                }
                
                case Qt::Key_Tab: {
                    // Tab to select first suggestion
                    if (suggestions_popup->rowCount() > 0) {
                        selectSuggestion(0);
                    }
                    return true; // Event handled
                }
                
                default:
                    break;
            }
        }
    }
    
    // Handle suggestions widget mouse events
    if (obj == suggestions && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex index = suggestions->indexAt(mouseEvent->pos());
        
        if (index.isValid()) {
            qDebug() << "MainWindow: Mouse click on suggestion:" << index.row();
            selectSuggestion(index.row());
            return true;
        }
    }
    
    // Pass the event to the base class
    return QMainWindow::eventFilter(obj, event);
}
