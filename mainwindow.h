#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
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

#include <QProcess>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

#include <QtGlobal>

#include <QApplication>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QTextEdit* input;
    QLineEdit* command;

    QAction* goToCommandBox;

    QVBoxLayout* layout;

    QProcess* server;

    QListView* suggestions;
    QStringListModel* suggestions_popup;

    QNetworkAccessManager* manager;
    
    // Server status tracking
    bool serverReady;
    QTimer* serverCheckTimer;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onPressCtrlSlash();
    void commandTextEdited();
    void getSuggestions(QString& query);
    void onSuggestionClicked(const QModelIndex &index);
    void displaySuggestions (QJsonArray& sugs);
    void checkServerHealth();
};
#endif // MAINWINDOW_H
