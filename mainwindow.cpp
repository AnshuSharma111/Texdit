#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // initialisations
    QClipboard *clipboard = QGuiApplication::clipboard();
    QWidget *centralWidget = new QWidget(this);
    command = new QLineEdit(this);
    input = new QTextEdit(this);
    layout = new QVBoxLayout(centralWidget);

    goToCommandBox = new QAction(tr("goToCommandBox"), this);

    // stylings
    input->setPlaceholderText("Enter text here...");
    input->setText(clipboard->text());
    command->setPlaceholderText("Ctrl + /");

    goToCommandBox->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    addAction(goToCommandBox);

    // connections
    connect(goToCommandBox, &QAction::triggered, this, &MainWindow::onPressCtrlSlash);
    connect(command, &QLineEdit::textChanged, this, &MainWindow::commandTextEdited);

    // placing in layout
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
    if (!text.isEmpty() && text[0] == "/") {
        qDebug() << "Eligible for suggestions";
    }
    else {
        qDebug() << "Nigga shut yo hoe ass back up!";
    }
}

MainWindow::~MainWindow() {}
