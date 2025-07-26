#include "mainwindow.h"
#include "loadingscreen.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Ensure server cleanup on application exit
    QObject::connect(&a, &QApplication::aboutToQuit, []() {
        LoadingScreen::stopServerProcess();
    });
    
    // Create loading screen first
    LoadingScreen *loadingScreen = new LoadingScreen();
    loadingScreen->show();
    
    // Create main window but don't show it yet
    MainWindow *mainWindow = nullptr;
    
    // Connect loading screen signals
    QObject::connect(loadingScreen, &LoadingScreen::serverReady, [&]() {
        // Server is ready, create and show main window
        mainWindow = new MainWindow();
        mainWindow->setFixedSize(1000, 900);
        mainWindow->show();
        
        // Hide and delete loading screen
        loadingScreen->hide();
        loadingScreen->deleteLater();
    });
    
    QObject::connect(loadingScreen, &LoadingScreen::serverFailed, [&]() {
        // Server failed, but still show main window
        mainWindow = new MainWindow();
        mainWindow->setFixedSize(1000, 900);
        mainWindow->show();
        
        // Hide and delete loading screen
        loadingScreen->hide();
        loadingScreen->deleteLater();
    });
    
    return a.exec();
}
