#include "servermanager.h"
#include <QDebug>
#include <QTimer>
#include <QNetworkRequest>

const QString ServerManager::SERVER_BASE_URL = "http://127.0.0.1:5000";
const int ServerManager::HEALTH_CHECK_INTERVAL = 1000; // 1 second for more responsive feedback
const int ServerManager::REQUEST_TIMEOUT = 8000; // 8 seconds for model loading
const int ServerManager::MAX_RETRY_ATTEMPTS = 15; // 15 attempts = 15 seconds total

ServerManager::ServerManager(QObject *parent)
    : QObject(parent)
    , currentStatus(Disconnected)
    , currentHealthCheck(nullptr)
    , consecutiveFailures(0)
{
    setupNetworkManager();
    
    // Setup health check timer
    healthCheckTimer = new QTimer(this);
    healthCheckTimer->setSingleShot(false);
    connect(healthCheckTimer, &QTimer::timeout, this, &ServerManager::performHealthCheck);
    
    qDebug() << "ServerManager: Initialized";
}

ServerManager::~ServerManager()
{
    stopHealthMonitoring();
    
    if (currentHealthCheck) {
        currentHealthCheck->abort();
        currentHealthCheck->deleteLater();
    }
}

void ServerManager::setupNetworkManager()
{
    networkManager = new QNetworkAccessManager(this);
    
    // Configure network manager for optimal performance
    networkManager->setTransferTimeout(REQUEST_TIMEOUT);
}

void ServerManager::setStatus(ServerStatus status)
{
    if (currentStatus != status) {
        currentStatus = status;
        qDebug() << "ServerManager: Status changed to" << status;
        emit statusChanged(status);
        
        if (status == Connected) {
            consecutiveFailures = 0;
            emit serverReady();
        }
    }
}

void ServerManager::startHealthMonitoring()
{
    qDebug() << "ServerManager: Starting health monitoring";
    setStatus(Connecting);
    
    // Perform immediate health check
    performHealthCheck();
    
    // Start periodic health checks
    healthCheckTimer->start(HEALTH_CHECK_INTERVAL);
}

void ServerManager::stopHealthMonitoring()
{
    qDebug() << "ServerManager: Stopping health monitoring";
    healthCheckTimer->stop();
    
    if (currentHealthCheck) {
        currentHealthCheck->abort();
        currentHealthCheck = nullptr;
    }
}

void ServerManager::performHealthCheck()
{
    // Don't start new health check if one is already running
    if (currentHealthCheck && currentHealthCheck->isRunning()) {
        qDebug() << "ServerManager: Health check already in progress, skipping";
        return;
    }
    
    QUrl url(SERVER_BASE_URL + "/health");
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "TexEdit-ServerManager");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // Set reasonable timeout
    request.setTransferTimeout(5000);
    
    currentHealthCheck = networkManager->get(request);
    
    connect(currentHealthCheck, &QNetworkReply::finished, 
            this, &ServerManager::handleHealthCheckResponse);
    
    // Backup timeout in case transfer timeout doesn't work
    QTimer::singleShot(6000, this, [this]() {
        if (currentHealthCheck && currentHealthCheck->isRunning()) {
            qDebug() << "ServerManager: Health check timeout, aborting";
            currentHealthCheck->abort();
        }
    });
}

void ServerManager::handleHealthCheckResponse()
{
    if (!currentHealthCheck) {
        return;
    }
    
    QNetworkReply::NetworkError error = currentHealthCheck->error();
    
    if (error == QNetworkReply::NoError) {
        // Health check successful
        consecutiveFailures = 0;
        
        if (currentStatus != Connected) {
            qDebug() << "ServerManager: ✅ Server is healthy and ready";
            setStatus(Connected);
        }
    } else {
        // Health check failed
        consecutiveFailures++;
        qDebug() << "ServerManager: ❌ Health check failed:" << currentHealthCheck->errorString() 
                 << "(consecutive failures:" << consecutiveFailures << ")";
        
        if (consecutiveFailures >= MAX_RETRY_ATTEMPTS) {
            setStatus(Error);
            emit serverError(QString("Server unreachable after %1 attempts: %2")
                           .arg(MAX_RETRY_ATTEMPTS)
                           .arg(currentHealthCheck->errorString()));
        } else {
            setStatus(Connecting);
        }
    }
    
    currentHealthCheck->deleteLater();
    currentHealthCheck = nullptr;
}

void ServerManager::makeRequest(const QString& endpoint, const QJsonObject& data,
                               std::function<void(const QJsonObject&)> onSuccess,
                               std::function<void(const QString&)> onError)
{
    if (currentStatus != Connected) {
        QString errorMsg = "Server not available for requests";
        qWarning() << "ServerManager:" << errorMsg;
        if (onError) {
            onError(errorMsg);
        }
        return;
    }
    
    QUrl url(SERVER_BASE_URL + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "TexEdit-Client");
    request.setTransferTimeout(REQUEST_TIMEOUT);
    
    QJsonDocument doc(data);
    QByteArray requestData = doc.toJson();
    
    QNetworkReply* reply = networkManager->post(request, requestData);
    
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Parse successful response
            QByteArray responseData = reply->readAll();
            QJsonParseError parseError;
            QJsonDocument responseDoc = QJsonDocument::fromJson(responseData, &parseError);
            
            if (parseError.error == QJsonParseError::NoError && responseDoc.isObject()) {
                if (onSuccess) {
                    onSuccess(responseDoc.object());
                }
            } else {
                QString errorMsg = QString("Invalid JSON response: %1").arg(parseError.errorString());
                qWarning() << "ServerManager:" << errorMsg;
                if (onError) {
                    onError(errorMsg);
                }
            }
        } else {
            // Handle network error
            QString errorMsg = QString("Network error: %1").arg(reply->errorString());
            qWarning() << "ServerManager:" << errorMsg;
            
            // If we get network errors, the server might be down
            consecutiveFailures++;
            if (consecutiveFailures >= MAX_RETRY_ATTEMPTS && currentStatus == Connected) {
                setStatus(Error);
            }
            
            if (onError) {
                onError(errorMsg);
            }
        }
        
        reply->deleteLater();
    });
}
