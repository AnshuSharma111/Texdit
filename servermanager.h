#ifndef SERVERMANAGER_H
#define SERVERMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>

class ServerManager : public QObject
{
    Q_OBJECT

public:
    enum ServerStatus {
        Disconnected,
        Connecting,
        Connected,
        Error
    };

    explicit ServerManager(QObject *parent = nullptr);
    ~ServerManager();

    // Server status
    ServerStatus getStatus() const { return currentStatus; }
    bool isReady() const { return currentStatus == Connected; }
    
    // Server operations
    void startHealthMonitoring();
    void stopHealthMonitoring();
    void makeRequest(const QString& endpoint, const QJsonObject& data, 
                    std::function<void(const QJsonObject&)> onSuccess,
                    std::function<void(const QString&)> onError = nullptr);

signals:
    void statusChanged(ServerStatus status);
    void serverReady();
    void serverError(const QString& error);

private slots:
    void performHealthCheck();
    void handleHealthCheckResponse();

private:
    void setStatus(ServerStatus status);
    void setupNetworkManager();
    
    QNetworkAccessManager* networkManager;
    QTimer* healthCheckTimer;
    ServerStatus currentStatus;
    QNetworkReply* currentHealthCheck;
    
    // Configuration
    static const QString SERVER_BASE_URL;
    static const int HEALTH_CHECK_INTERVAL;
    static const int REQUEST_TIMEOUT;
    static const int MAX_RETRY_ATTEMPTS;
    
    int consecutiveFailures;
};

#endif // SERVERMANAGER_H
