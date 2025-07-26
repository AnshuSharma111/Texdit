#ifndef LOADINGSCREEN_H
#define LOADINGSCREEN_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMovie>
#include <QTimer>
#include <QProgressBar>
#include <QPushButton>
#include <QProcess>
#include <QPointer>

// Forward declaration
class ServerManager;

class LoadingScreen : public QWidget
{
    Q_OBJECT

public:
    LoadingScreen(QWidget *parent = nullptr);
    ~LoadingScreen();
    
    // Static server process management
    static QProcess* getServerProcess();
    static void startServerProcess();
    static void stopServerProcess();

signals:
    void serverReady();
    void serverFailed();

private slots:
    void onServerStatusChanged(int status);
    void onRetryClicked();
    void updateLoadingText();

private:
    void setupUI();
    void initializeServer();
    void showRetryOption();
    void hideRetryOption();
    
    QVBoxLayout *mainLayout;
    QHBoxLayout *loadingLayout;
    QLabel *titleLabel;
    QLabel *statusLabel;
    QLabel *loadingIcon;
    QProgressBar *progressBar;
    QPushButton *retryButton;
    QPushButton *skipButton;
    
    QTimer *animationTimer;
    ServerManager *serverManager;
    
    int animationStep;
    QStringList loadingTexts;
    
    // Static server process
    static QProcess *globalServerProcess;
};

#endif // LOADINGSCREEN_H
