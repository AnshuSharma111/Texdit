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
private slots:
    void onPressCtrlSlash();
    void commandTextEdited();
};
#endif // MAINWINDOW_H
