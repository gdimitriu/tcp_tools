/*
 * Robot Controll
 *
 * Copyright 2024 Gabriel Dimitriu
 *
 * This file is part of tcp_tools project.

 * tcp_tools is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * tcp_tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tcp_tools; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
*/
#include "robotcontroll.h"
#include "ui_robotcontroll.h"
#include <QMessageBox>

RobotControll::RobotControll(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RobotControll)
{
    ui->setupUi(this);
    socket = nullptr;    
    currentButton = nullptr;
    isCurrentPowerChanges = false;
    init();
    isMinMaxPowerChanged = MIN_MAX_POWER_CHANGES::NONE;
}

RobotControll::~RobotControll()
{
    if ( socket != nullptr ) {
        socket->close();
        delete socket;
    }
    delete ui;
}

void RobotControll::init()
{
    //socket communication
    ui->statusValue->setText("Disconnected");
    connect(ui->connectButton, SIGNAL(clicked(bool)), this, SLOT(connectTo()));
    connect(ui->disconnectButton, SIGNAL(clicked(bool)), this, SLOT(disconnectFrom()));
    //setup line termination
    ui->lineTermination->addItem("None", QVariant::fromValue(LINE_TERMINATION::NONE));
    ui->lineTermination->addItem("LF", QVariant::fromValue(LINE_TERMINATION::LF));
    ui->lineTermination->addItem("CR", QVariant::fromValue(LINE_TERMINATION::CR));
    ui->lineTermination->addItem("CR&LF", QVariant::fromValue(LINE_TERMINATION::CR_LF));

    // moving buttons
    connect(ui->forwardButton, SIGNAL(pressed()), this, SLOT(forward()));
    connect(ui->forwardButton, SIGNAL(released()), this, SLOT(stop()));
    connect(ui->backwardButton, SIGNAL(pressed()), this, SLOT(backward()));
    connect(ui->backwardButton, SIGNAL(released()), this, SLOT(stop()));
    connect(ui->leftButton, SIGNAL(pressed()), this, SLOT(left()));
    connect(ui->leftButton, SIGNAL(released()), this, SLOT(stop()));
    connect(ui->rightButton, SIGNAL(pressed()), this, SLOT(right()));
    connect(ui->rightButton, SIGNAL(released()), this, SLOT(stop()));
    connect(ui->stopButton, SIGNAL(clicked(bool)), this, SLOT(stop()));

    // power level
    ui->maximumPower->setText("4095");
    ui->minimumPower->setText("1000");
    ui->powerLevel->setRange(ui->minimumPower->text().toInt(),ui->maximumPower->text().toInt());
    connect(ui->powerLevel, SIGNAL(sliderMoved(int)), this, SLOT(powerChanged(int)));

    connect(ui->minimumPower, SIGNAL(editingFinished()), this, SLOT(minPowerChanged()));
    connect(ui->maximumPower, SIGNAL(editingFinished()), this, SLOT(maxPowerChanged()));

    //fetch config from Robot
    connect(ui->fetchButton, SIGNAL(clicked(bool)), this, SLOT(fetchFromRobot()));
    //deploy config to Robot
    connect(ui->deployButton, SIGNAL(clicked(bool)), this, SLOT(deployToRobot()));
}


void RobotControll::connectTo()
{
    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(sockConnected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(sockDisconnected()));
    socket->connectToHost(ui->ipValue->text(), ui->portValue->text().toInt());
    ui->connectButton->clearFocus();
}

void RobotControll::disconnectFrom()
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

void RobotControll::sockDisconnected()
{
    ui->statusValue->setText("Disconnected");
    repaint();
}

void RobotControll::sockConnected()
{
    ui->statusValue->setText("Connected");
    repaint();
}

void RobotControll::forward()
{
    sendPowerToRobotIfModified();
    sendOneWay("M1,0#");
    currentButton = ui->forwardButton;
}

void RobotControll::backward()
{
    sendPowerToRobotIfModified();
    sendOneWay("M-1,0#");
    currentButton = ui->backwardButton;
}

void RobotControll::left()
{
    sendPowerToRobotIfModified();
    sendOneWay("M0,-1#");
    currentButton = ui->leftButton;
}

void RobotControll::right()
{
    sendPowerToRobotIfModified();
    sendOneWay("M0,1#");
    currentButton = ui->rightButton;
}

void RobotControll::stop()
{
    sendOneWay("b#");
    if ( currentButton != nullptr )
    {
        currentButton->clearFocus();
        currentButton = nullptr;
    }
    else
    {
        ui->stopButton->clearFocus();
    }
}

void RobotControll::powerChanged(int value)
{
    ui->currentPowerLevel->setText(QString::number(value));
    repaint();
    isCurrentPowerChanges = true;
}

QString RobotControll::sendWithReply(QString message)
{
    if ( socket != nullptr )
    {
        switch ( ui->lineTermination->currentData().value<LINE_TERMINATION>() )
        {
        case LINE_TERMINATION::LF :
            message.append('\n');
            break;
        case LINE_TERMINATION::CR :
            message.append('\r');
            break;
        case LINE_TERMINATION::CR_LF :
            message.append("\r\n");
            break;
        default:
            break;
        }
        socket->write(message.toLatin1());
        socket->flush();
        socket->waitForReadyRead();
        QByteArray readData = socket->readAll();
        while ( socket->bytesAvailable() > 0) {
            QByteArray localReadData = socket->readAll();
            if ( readData.isEmpty() )
                break;
            readData.append(localReadData);
        }
        return QString(readData);
    }
    else
    {
        QMessageBox::critical(this, tr("First connect to server."), tr("First connect to server"), QMessageBox::Ok);
    }
    return "";
}

void RobotControll::sendOneWay(QString message, bool hasAck)
{
    if ( socket != nullptr )
    {
        switch ( (LINE_TERMINATION) ui->lineTermination->currentData().value<LINE_TERMINATION>() )
        {
        case LINE_TERMINATION::LF :
            message.append('\n');
            break;
        case LINE_TERMINATION::CR :
            message.append('\r');
            break;
        case LINE_TERMINATION::CR_LF :
            message.append("\r\n");
            break;
        default:
            break;
        }
        socket->write(message.toLatin1());
        socket->flush();
        if ( hasAck )
        {
            socket->waitForReadyRead();
            QByteArray readData = socket->readAll();
            QString str(readData);
            while ( !str.contains("OK\r\n") )
            {
                socket->waitForReadyRead();
                str.append(socket->readAll());
            }
        }
    }
    else
    {
        QMessageBox::critical(this, tr("First connect to server."), tr("First connect to server"), QMessageBox::Ok);
    }
}

void RobotControll::deployToRobot()
{
    QString value;
    value.append("V");
    value.append(ui->maximumPower->text());
    value.append('#');
    sendOneWay(value, true);
    value.clear();
    value.append("v");
    value.append(ui->minimumPower->text());
    value.append('#');
    sendOneWay(value, true);
    value.clear();
    value.append("c");
    value.append(ui->currentPowerLevel->text());
    value.append('#');
    sendOneWay(value, true);
    ui->deployButton->clearFocus();
}

void RobotControll::fetchFromRobot()
{
    QString value = sendWithReply("V#");
    ui->maximumPower->setText(value.trimmed());
    value = sendWithReply("v#");
    ui->minimumPower->setText(value.trimmed());
    value = sendWithReply("c#");
    ui->currentPowerLevel->setText(value.trimmed());
    ui->powerLevel->setRange(ui->minimumPower->text().toInt(),ui->maximumPower->text().toInt());
    ui->powerLevel->setValue(ui->minimumPower->text().toInt());
    isMinMaxPowerChanged = MIN_MAX_POWER_CHANGES::NONE;
    ui->fetchButton->clearFocus();
}

void RobotControll::minPowerChanged()
{
    ui->powerLevel->setMinimum(ui->minimumPower->text().toInt());
    if ( ui->powerLevel->minimum() > ui->currentPowerLevel->text().toInt() )
    {
        ui->currentPowerLevel->setText( QString::number(ui->powerLevel->minimum()));
        isCurrentPowerChanges = true;
    }
    if ( isMinMaxPowerChanged == MIN_MAX_POWER_CHANGES::NONE )
        isMinMaxPowerChanged = MIN_MAX_POWER_CHANGES::MIN;
    else
        isMinMaxPowerChanged = MIN_MAX_POWER_CHANGES::MINMAX;
}

void RobotControll::maxPowerChanged()
{
    ui->powerLevel->setMaximum(ui->maximumPower->text().toInt());
    if ( ui->powerLevel->maximum() < ui->currentPowerLevel->text().toInt() )
    {
        ui->currentPowerLevel->setText(QString::number(ui->powerLevel->maximum()));
        isCurrentPowerChanges = true;
    }
    if ( isMinMaxPowerChanged == MIN_MAX_POWER_CHANGES::NONE )
        isMinMaxPowerChanged = MIN_MAX_POWER_CHANGES::MAX;
    else
        isMinMaxPowerChanged = MIN_MAX_POWER_CHANGES::MINMAX;
}

void RobotControll::sendPowerToRobotIfModified()
{
    QString value;
    switch( isMinMaxPowerChanged )
    {
    case MIN_MAX_POWER_CHANGES::MAX:
        value.clear();
        value.append("V");
        value.append(ui->maximumPower->text());
        value.append('#');
        sendOneWay(value, true);
        break;
    case MIN_MAX_POWER_CHANGES::MIN:
        value.clear();
        value.append("v");
        value.append(ui->minimumPower->text());
        value.append('#');
        sendOneWay(value, true);
        break;
    case MIN_MAX_POWER_CHANGES::MINMAX:
        value.clear();
        value.append("V");
        value.append(ui->maximumPower->text());
        value.append('#');
        sendOneWay(value, true);
        value.clear();
        value.append("v");
        value.append(ui->minimumPower->text());
        value.append('#');
        sendOneWay(value, true);
        break;
    default:
        break;
    }
    isMinMaxPowerChanged = MIN_MAX_POWER_CHANGES::NONE;
    if ( isCurrentPowerChanges )
    {
        value.clear();
        value.append("c");
        value.append(ui->currentPowerLevel->text());
        value.append('#');
        sendOneWay(value, true);
        isCurrentPowerChanges = false;
    }
}
