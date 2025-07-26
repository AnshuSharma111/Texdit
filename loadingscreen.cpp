#include "loadingscreen.h"
#include "servermanager.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QPointer>

// Static server process definition
QProcess* LoadingScreen::globalServerProcess = nullptr;

LoadingScreen::LoadingScreen(QWidget *parent)
    : QWidget(parent)
    , animationStep(0)
{
    setupUI();
    
    // Initialize server manager
    serverManager = new ServerManager(this);
    
    // Setup loading text animation
    loadingTexts = {
        "Starting Python server...",
        "Loading TensorFlow models...",
        "Initializing AI components...",
        "Starting Flask application...",
        "Almost ready...",
        "Checking server health...",
        "Connecting to backend..."
    };
    
    // Start text animation
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &LoadingScreen::updateLoadingText);
    animationTimer->start(1500); // Change text every 1.5 seconds for better feedback
    
    // Connect to server manager signals
    connect(serverManager, &ServerManager::statusChanged, this, &LoadingScreen::onServerStatusChanged);
    
    // Start the server first, then begin health checking
    qDebug() << "LoadingScreen: Starting Python server...";
    
    // Use QPointer for safe timer callback
    QPointer<LoadingScreen> self(this);
    QTimer::singleShot(1000, [self]() {
        if (self) {
            self->initializeServer();
        }
    });
    
    qDebug() << "LoadingScreen: Initialized and starting server...";
}

LoadingScreen::~LoadingScreen()
{
    // Stop any pending timers that might reference this object
    if (animationTimer) {
        animationTimer->stop();
    }
    
    // Server process is managed globally, don't stop it here
    qDebug() << "LoadingScreen: Destroyed, server continues running";
}

// Static server process management
QProcess* LoadingScreen::getServerProcess()
{
    return globalServerProcess;
}

void LoadingScreen::startServerProcess()
{
    if (globalServerProcess && globalServerProcess->state() == QProcess::Running) {
        qDebug() << "LoadingScreen: Server process already running";
        return;
    }
    
    if (!globalServerProcess) {
        globalServerProcess = new QProcess();
    }
    
    qDebug() << "LoadingScreen: Starting global server process";
    globalServerProcess->start("python", QStringList() << "../../backend/server.py");
    
    QObject::connect(globalServerProcess, &QProcess::readyReadStandardOutput, []() {
        qDebug() << "Server output:" << globalServerProcess->readAllStandardOutput();
    });
    
    QObject::connect(globalServerProcess, &QProcess::readyReadStandardError, []() {
        qDebug() << "Server error:" << globalServerProcess->readAllStandardError();
    });
    
    QObject::connect(globalServerProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), 
                     [](int exitCode, QProcess::ExitStatus exitStatus) {
        qDebug() << "Server process finished with exit code:" << exitCode << "status:" << exitStatus;
    });
}

void LoadingScreen::stopServerProcess()
{
    if (globalServerProcess && globalServerProcess->state() == QProcess::Running) {
        qDebug() << "LoadingScreen: Stopping global server process";
        
        // Try graceful termination first
        globalServerProcess->terminate();
        if (!globalServerProcess->waitForFinished(3000)) {
            qDebug() << "LoadingScreen: Graceful termination failed, forcing kill";
            globalServerProcess->kill();
            globalServerProcess->waitForFinished(2000);
        }
        
        qDebug() << "LoadingScreen: Server process cleanup completed";
    }
}

void LoadingScreen::initializeServer()
{
    startServerProcess();
    
    // Show progress during startup wait
    statusLabel->setText("Server starting, loading AI models...");
    progressBar->setRange(0, 8); // 8 seconds progress
    progressBar->setValue(0);
    
    // Use QPointer for safe object access in timer callbacks
    QPointer<LoadingScreen> self(this);
    
    // Update progress every second
    QTimer *progressTimer = new QTimer(this);
    connect(progressTimer, &QTimer::timeout, [self, progressTimer]() {
        if (!self) return; // Object was destroyed
        int currentValue = self->progressBar->value();
        if (currentValue < self->progressBar->maximum()) {
            self->progressBar->setValue(currentValue + 1);
        } else {
            progressTimer->stop();
            progressTimer->deleteLater();
        }
    });
    progressTimer->start(1000);
    
    // Use QTimer with parent for safer cleanup
    QTimer *healthTimer = new QTimer(this);
    healthTimer->setSingleShot(true);
    connect(healthTimer, &QTimer::timeout, [self]() {
        if (!self) return; // Object was destroyed
        self->statusLabel->setText("Server started, checking health...");
        self->progressBar->setRange(0, 0); // Back to indeterminate for health checks
        self->serverManager->startHealthMonitoring();
    });
    healthTimer->start(8000);
}

void LoadingScreen::setupUI()
{
    setWindowTitle("TexEdit - Loading");
    setFixedSize(400, 300);
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    
    // Center the window on screen
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    // Setup layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    
    // Title
    titleLabel = new QLabel("TexEdit");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 24px;"
        "    font-weight: bold;"
        "    color: #2c3e50;"
        "    margin-bottom: 10px;"
        "}"
    );
    
    // Loading animation area
    loadingLayout = new QHBoxLayout();
    loadingLayout->setAlignment(Qt::AlignCenter);
    
    // Create a simple text-based loading animation since we don't have a GIF
    loadingIcon = new QLabel("⏳");
    loadingIcon->setAlignment(Qt::AlignCenter);
    loadingIcon->setStyleSheet(
        "QLabel {"
        "    font-size: 32px;"
        "    margin-right: 10px;"
        "}"
    );
    
    loadingLayout->addWidget(loadingIcon);
    
    // Status label
    statusLabel = new QLabel("Initializing server...");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 14px;"
        "    color: #7f8c8d;"
        "    margin: 10px 0;"
        "}"
    );
    
    // Progress bar
    progressBar = new QProgressBar();
    progressBar->setRange(0, 0); // Indeterminate progress
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #bdc3c7;"
        "    border-radius: 5px;"
        "    text-align: center;"
        "    font-size: 12px;"
        "}"
        "QProgressBar::chunk {"
        "    background-color: #3498db;"
        "    border-radius: 3px;"
        "}"
    );
    
    // Retry button (initially hidden)
    retryButton = new QPushButton("Retry");
    retryButton->setVisible(false);
    retryButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 20px;"
        "    border-radius: 5px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980b9;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #21618c;"
        "}"
    );
    
    // Skip button (initially hidden)
    skipButton = new QPushButton("Skip Server Check");
    skipButton->setVisible(false);
    skipButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #95a5a6;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 20px;"
        "    border-radius: 5px;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7f8c8d;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #6c7b7d;"
        "}"
    );
    
    // Connect buttons
    connect(retryButton, &QPushButton::clicked, this, &LoadingScreen::onRetryClicked);
    connect(skipButton, &QPushButton::clicked, this, &LoadingScreen::serverReady);
    
    // Add widgets to layout
    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(loadingLayout);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addStretch();
    
    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(retryButton);
    buttonLayout->addWidget(skipButton);
    buttonLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
    
    // Apply overall styling
    setStyleSheet(
        "QWidget {"
        "    background-color: #ecf0f1;"
        "    font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
    );
}

void LoadingScreen::onServerStatusChanged(int status)
{
    switch (status) {
        case ServerManager::Connected:
            qDebug() << "LoadingScreen: ✅ Server is ready!";
            animationTimer->stop();
            
            statusLabel->setText("Server ready! Starting application...");
            loadingIcon->setText("✅");
            progressBar->setRange(0, 100);
            progressBar->setValue(100);
            
            // Brief delay before emitting ready signal
            QTimer::singleShot(1000, this, [=]() {
                emit serverReady();
            });
            break;
            
        case ServerManager::Connecting:
            statusLabel->setText("Connecting to server...");
            hideRetryOption();
            break;
            
        case ServerManager::Disconnected:
            statusLabel->setText("Checking server availability...");
            break;
            
        case ServerManager::Error:
            qDebug() << "LoadingScreen: Server connection failed, showing retry option";
            animationTimer->stop();
            showRetryOption();
            break;
    }
}

void LoadingScreen::updateLoadingText()
{
    if (serverManager && serverManager->getStatus() != ServerManager::Connected) {
        QString currentText = loadingTexts[animationStep % loadingTexts.size()];
        statusLabel->setText(currentText);
        animationStep++;
        
        // Animate loading icon rotation
        static QStringList loadingIcons = {"⏳", "⌛", "🔄", "⚡"};
        loadingIcon->setText(loadingIcons[animationStep % loadingIcons.size()]);
    }
}

void LoadingScreen::onRetryClicked()
{
    qDebug() << "LoadingScreen: Retry clicked, restarting server and health check";
    hideRetryOption();
    
    // Restart animations
    animationTimer->start(1500);
    
    statusLabel->setText("Restarting server...");
    loadingIcon->setText("⏳");
    progressBar->setRange(0, 0); // Back to indeterminate
    
    // Stop any existing server process and restart
    stopServerProcess();
    QTimer::singleShot(2000, [this]() {
        initializeServer();
    });
}

void LoadingScreen::showRetryOption()
{
    statusLabel->setText("❌ Server connection failed");
    loadingIcon->setText("❌");
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    
    retryButton->setVisible(true);
    skipButton->setVisible(true);
    
    qDebug() << "LoadingScreen: Showing retry options";
}

void LoadingScreen::hideRetryOption()
{
    retryButton->setVisible(false);
    skipButton->setVisible(false);
}
