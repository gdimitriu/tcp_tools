#include "tcpterminal.h"
#include "ui_tcpterminal.h"
#include <QMessageBox>


TcpTerminal::TcpTerminal(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TcpTerminal)
{
    ui->setupUi(this);
    init();
    socket = nullptr;
    ui->statusValue->setText("Disconnected");
}

TcpTerminal::~TcpTerminal()
{
    if ( socket != nullptr ) {
        socket->close();
        delete socket;
    }
    delete ui;
}

void TcpTerminal::init()
{
    connect(ui->connectButton, SIGNAL(clicked(bool)), this, SLOT(connectTo()));
    connect(ui->disconnectButton, SIGNAL(clicked(bool)), this, SLOT(disconnectFrom()));
    connect(ui->sendButton, SIGNAL(clicked(bool)), this, SLOT(sendData()));
    connect(ui->clearButton, SIGNAL(clicked(bool)), this, SLOT(clearReceiveData()));

}

void TcpTerminal::connectTo()
{
    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(sockConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(sockDisconnected()));
    socket->connectToHost(ui->ipValue->text(), ui->portValue->text().toInt());
    ui->connectButton->clearFocus();
}

void TcpTerminal::disconnectFrom()
{
    if ( socket != nullptr )
    {
        socket->disconnectFromHost();
        socket->close();
        delete socket;
        socket = nullptr;
    }
    ui->disconnectButton->clearFocus();
}

void TcpTerminal::sockDisconnected()
{
    ui->statusValue->setText("Disconnected");
    repaint();
}

void TcpTerminal::sockConnected()
{
    ui->statusValue->setText("Connected");
    repaint();
}

void TcpTerminal::clearReceiveData()
{
    ui->receiveData->clear();
}

void TcpTerminal::sendData()
{
    QString str = ui->sendValue->text();
    if ( ui->lfTerminator->isChecked() )
        str = str + "\n";
    else if ( ui->crTerminator->isChecked() )
        str = str + "\r";
    else if ( ui->crlfTerminator->isChecked() )
        str = str + "\r\n";
    if ( socket != nullptr)
    {
        socket->write(str.toUtf8());
        socket->flush();
        if ( ui->sendWithResponse->isChecked() )
        {
            socket->waitForReadyRead();
            QByteArray readData = socket->readLine();
            socket->flush();
            ui->receiveData->moveCursor(QTextCursor::End);
            ui->receiveData->insertPlainText(QString::fromUtf8(readData));
            while ( socket->bytesAvailable() > 0) {
                readData = socket->readLine();
                if ( readData.isEmpty() )
                    break;
                ui->receiveData->moveCursor(QTextCursor::End);
                ui->receiveData->insertPlainText(QString::fromUtf8(readData));
            }
            QMainWindow::repaint();
            ui->sendButton->clearFocus();
        }
    }
    else
    {
        QMessageBox::critical(this, tr("First connect to server."), tr("First connect to server"), QMessageBox::Ok);
    }
}
