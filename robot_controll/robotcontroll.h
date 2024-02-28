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

#ifndef ROBOTCONTROLL_H
#define ROBOTCONTROLL_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QPushButton>

namespace Ui {
class RobotControll;
}

class RobotControll : public QMainWindow
{
    Q_OBJECT
private slots:
    void connectTo();
    void disconnectFrom();
    void sockConnected();
    void sockDisconnected();
    void forward();
    void backward();
    void left();
    void right();
    void stop();
    void powerChanged(int value);
    void deployToRobot();
    void fetchFromRobot();
    void minPowerChanged();
    void maxPowerChanged();
public:
    explicit RobotControll(QWidget *parent = 0);
    ~RobotControll();
    enum class LINE_TERMINATION { NONE = 0, LF = 1, CR = 2, CR_LF = 3};
    Q_ENUM(LINE_TERMINATION)
    enum class MIN_MAX_POWER_CHANGES { NONE = 0, MIN = 1, MAX = 2, MINMAX = 3};
    Q_ENUM(MIN_MAX_POWER_CHANGES)
private:
    void init();
    QString sendWithReply(QString message);
    void sendOneWay(QString message, bool hasAck = false);
    QTcpSocket *socket;
    Ui::RobotControll *ui;
    QPushButton *currentButton;
    bool isCurrentPowerChanges;
    MIN_MAX_POWER_CHANGES isMinMaxPowerChanged;
    void sendPowerToRobotIfModified();
};

#endif // ROBOTCONTROLL_H
