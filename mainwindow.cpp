#include "mainwindow.h"
#include "commandRegistry.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // initialisations
    QClipboard *clipboard = QGuiApplication::clipboard();
    QWidget *centralWidget = new QWidget(this);
    command = new QLineEdit(this);
    input = new QTextEdit(this);
    suggestions = new QListView(this);
    suggestions_popup = new QStringListModel();
    layout = new QVBoxLayout(centralWidget);

    server = new QProcess(this); // backend server
    serverReady = false; // Server not ready initially
    serverCheckTimer = new QTimer(this);

    manager = new QNetworkAccessManager(this); // network manager

    goToCommandBox = new QAction(tr("goToCommandBox"), this); // command box shortcut

    // stylings
    input->setPlaceholderText("Enter text here...");
    input->setText(clipboard->text());
    command->setPlaceholderText("Ctrl + /");

    suggestions->setWindowFlags(Qt::ToolTip);
    suggestions->setFocusPolicy(Qt::NoFocus); // Changed from StrongFocus to NoFocus
    suggestions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    suggestions->setSelectionMode(QAbstractItemView::SingleSelection);
    suggestions->setUniformItemSizes(true);
    suggestions->setMouseTracking(true); // Enable mouse tracking

    suggestions->setModel(suggestions_popup);

    // keybinds
    goToCommandBox->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    addAction(goToCommandBox);

    // connections
    connect(goToCommandBox, &QAction::triggered, this, &MainWindow::onPressCtrlSlash); // go to command box
    connect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited); // suggest commands
    
    // Multiple ways to capture suggestion selection
    connect(suggestions, &QListView::clicked, this, &MainWindow::onSuggestionClicked); // go through suggestions
    connect(suggestions, &QListView::activated, this, &MainWindow::onSuggestionClicked); // alternative activation
    connect(suggestions, &QAbstractItemView::pressed, this, &MainWindow::onSuggestionClicked); // mouse press
    
    // Install event filter to handle arrow keys and Enter in command box
    command->installEventFilter(this);
    
    // Also install event filter on suggestions to debug mouse events
    suggestions->installEventFilter(this);
    
    connect(qApp, &QApplication::focusChanged,this, [=](QWidget *old, QWidget *now) {
        if (suggestions->isVisible() && now != command) {
            suggestions->hide();
        }
    });

    // start python server
    qDebug() << "Starting Python server...";
    command->setEnabled(false); // Disable command input until server is ready
    command->setPlaceholderText("🔄 Loading server...");
    command->setStyleSheet("QLineEdit { color: #888; font-style: italic; }");
    
    server->start("python", QStringList() << "../../backend/server.py");
    connect (server, &QProcess::readyReadStandardOutput, this, [=] () { // check whether the server started properly
        qDebug() << "Server output:" << server->readAllStandardOutput();
    });
    connect(server, &QProcess::readyReadStandardError, this, [=] () {
        qDebug() << "Server error:" << server->readAllStandardError();
    });
    
    // Start health checking the server every 500ms
    connect(serverCheckTimer, &QTimer::timeout, this, &MainWindow::checkServerHealth);
    serverCheckTimer->start(500);

    // place widgets in layout
    layout->addWidget(input, 9);
    layout->addWidget(command, 1);

    // show central widget
    setCentralWidget(centralWidget);
}

void MainWindow::onPressCtrlSlash () {
    if (!command->hasFocus()) {
        command->setFocus(); // get focus to command terminal
        command->setText("/"); // pre-write / on the terminal

        command->setCursorPosition(1); // move cursor to after

        qDebug() << "Focus shifted to command TextEdit";
    }
}

void MainWindow::commandTextEdited () {
    QString text = command->text();
    qDebug() << "Command text changed to:" << text;
    
    if (!text.isEmpty() && text[0] == "/") {
        // Check if the text is already a complete command (avoid showing suggestions for complete matches)
        QStringList allCommands = commandRegistry::getAllCommands();
        if (!allCommands.contains(text)) {
            qDebug() << "Eligible for suggestions";
            getSuggestions(text);
        } else {
            qDebug() << "Complete command, hiding suggestions";
            suggestions->hide();
        }
    }
    else {
        qDebug() << "Ineligible";
        suggestions->hide(); // Hide suggestions if not eligible
    }
}

void MainWindow::displaySuggestions (QJsonArray& sugs) {
    qDebug() << "displaySuggestions called with:" << sugs;
    
    QStringList words;
    for (const QJsonValueRef &suggestion : sugs) {
        words.append(suggestion.toString());
    }

    qDebug() << "Converted to StringList:" << words;
    suggestions_popup->setStringList(words);

    if (words.isEmpty()) {
        qDebug() << "No suggestions to show, hiding popup";
        suggestions->hide();
        return;
    }

    int cursorPos = command->cursorPosition();
    QString textBeforeCursor = command->text().left(cursorPos);

    int textWidth = command->fontMetrics().horizontalAdvance(textBeforeCursor);
    QPoint caretPos(textWidth, command->height());  // Offset below the line

    QPoint globalCaretPos = command->mapToGlobal(caretPos);

    int rowCount = suggestions_popup->rowCount();
    int popupHeight = suggestions->sizeHintForRow(0) * qMin(rowCount, 6);

    suggestions->resize(200, popupHeight);
    suggestions->move(globalCaretPos);
    
    // Select the first item by default
    if (suggestions_popup->rowCount() > 0) {
        QModelIndex firstIndex = suggestions_popup->index(0, 0);
        suggestions->setCurrentIndex(firstIndex);
    }
    
    suggestions->show();
    
    qDebug() << "Showing suggestions popup at:" << globalCaretPos << "with size:" << QSize(200, popupHeight);
}

void MainWindow::getSuggestions (QString &query) {
    // Don't make requests if server isn't ready
    if (!serverReady) {
        qDebug() << "Server not ready, skipping suggestion request";
        return;
    }
    
    QUrl url("http://127.0.0.1:5000/api/search");

    QJsonObject json;
    QJsonArray choices;

    for(const QString& choice : commandRegistry::getAllCommands()) {
        choices.append(choice);
    }

    json["query"] = query;
    json["choices"] = choices;

    QJsonDocument doc(json);
    QByteArray data = doc.toJson();
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = manager->post(request, data);
    
    connect(reply, &QNetworkReply::finished, this, [=] () {
        QByteArray response = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(response);
        QJsonArray results;

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Network error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        if (!jsonDoc.isObject()) {
            qWarning() << "Invalid JSON: not an object";
            qWarning() << "Response was:" << response;
            reply->deleteLater();
            return;
        }

        QJsonObject root = jsonDoc.object();
        if (root.contains("results") && root["results"].isArray()) {
            results = root["results"].toArray();
            qDebug() << "Got results:" << results;
        }
        else {
            qDebug() << "No suggestions";
            qDebug() << "Response object:" << root;
        }

        reply->deleteLater();
        displaySuggestions(results);
    });
}

void MainWindow::checkServerHealth() {
    if (serverReady) {
        return; // Already ready, no need to check
    }
    
    QUrl url("http://127.0.0.1:5000/");
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "TexEditHealthCheck");
    
    QNetworkReply* reply = manager->get(request);
    
    // Set a short timeout for health checks
    QTimer::singleShot(2000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Server is ready!
            serverReady = true;
            serverCheckTimer->stop();
            
            qDebug() << "✅ Server is ready!";
            
            // Re-enable command input
            command->setEnabled(true);
            command->setPlaceholderText("Ctrl + /");
            command->setStyleSheet(""); // Remove any loading styles
            
            // Show a brief success message
            command->setPlaceholderText("Server ready - Ctrl + /");
            QTimer::singleShot(2000, this, [=]() {
                command->setPlaceholderText("Ctrl + /");
            });
            
        } else {
            // Server not ready yet, keep checking
            qDebug() << "⏳ Server not ready yet, retrying...";
        }
        
        reply->deleteLater();
    });
}


void MainWindow::onSuggestionClicked(const QModelIndex &index) {
    qDebug() << "Suggestion clicked! Index:" << index << "Row:" << index.row();
    
    if (!index.isValid()) {
        qDebug() << "Invalid index clicked";
        return;
    }
    
    QString selectedText = index.data(Qt::DisplayRole).toString();
    qDebug() << "Selected text:" << selectedText;
    
    // Hide suggestions first
    suggestions->hide();
    
    // Temporarily disconnect textChanged to prevent retriggering suggestions
    disconnect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);
    
    command->setText(selectedText);
    command->setCursorPosition(selectedText.length()); // Move cursor to end
    
    qDebug() << "Command text set to:" << command->text();
    
    // Reconnect after a brief delay
    QTimer::singleShot(50, this, [=]() {
        connect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);
    });
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // Handle command box key events
    if (obj == command && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        
        // Only handle navigation keys when suggestions are visible
        if (suggestions->isVisible()) {
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
                    // Select current suggestion
                    QModelIndex currentIndex = suggestions->currentIndex();
                    if (currentIndex.isValid()) {
                        QString selectedText = currentIndex.data(Qt::DisplayRole).toString();
                        
                        // Hide suggestions first to prevent re-triggering
                        suggestions->hide();
                        
                        // Temporarily disconnect textChanged to prevent retriggering suggestions
                        disconnect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);
                        
                        command->setText(selectedText);
                        command->setCursorPosition(selectedText.length()); // Move cursor to end
                        
                        // Reconnect after a brief delay
                        QTimer::singleShot(50, this, [=]() {
                            connect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);
                        });
                    }
                    return true; // Event handled
                }
                
                case Qt::Key_Escape: {
                    // Hide suggestions on Escape
                    suggestions->hide();
                    return true; // Event handled
                }
                
                default:
                    break;
            }
        }
    }
    
    // Handle suggestions widget mouse events for debugging
    if (obj == suggestions) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            qDebug() << "Mouse press on suggestions at:" << mouseEvent->pos();
            
            QModelIndex index = suggestions->indexAt(mouseEvent->pos());
            if (index.isValid()) {
                qDebug() << "Valid index under mouse:" << index.row();
                // Manually trigger the selection
                onSuggestionClicked(index);
                return true;
            }
        }
    }
    
    // Pass the event to the base class
    return QMainWindow::eventFilter(obj, event);
}

MainWindow::~MainWindow() {
    // Stop health checking
    if (serverCheckTimer) {
        serverCheckTimer->stop();
    }
    
    // kill server
    if (server && server->state() != QProcess::NotRunning) {
        server->kill();  // Forceful kill
        if (!server->waitForFinished(3000)) {
            qDebug() << "Server process did not exit in time";
        } else {
            qDebug() << "Killed server";
        }
    }
    manager->deleteLater();
}
