/********************************************************************************
** Form generated from reading UI file 'boatspeed.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BOATSPEED_H
#define UI_BOATSPEED_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_BoatSpeed
{
public:
    QWidget *centralwidget;
    QTextEdit *Log;
    QLabel *Graph;
    QLabel *label_2;
    QLabel *Speed;
    QLabel *label_4;
    QLabel *Last;
    QLabel *Count;
    QLabel *label_5;
    QLabel *label_3;
    QLabel *maxSpeed;
    QPushButton *Reset;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *BoatSpeed)
    {
        if (BoatSpeed->objectName().isEmpty())
            BoatSpeed->setObjectName(QString::fromUtf8("BoatSpeed"));
        BoatSpeed->resize(1800, 1500);
        centralwidget = new QWidget(BoatSpeed);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        Log = new QTextEdit(centralwidget);
        Log->setObjectName(QString::fromUtf8("Log"));
        Log->setGeometry(QRect(20, 790, 851, 411));
        QFont font;
        font.setPointSize(7);
        Log->setFont(font);
        Graph = new QLabel(centralwidget);
        Graph->setObjectName(QString::fromUtf8("Graph"));
        Graph->setGeometry(QRect(20, 180, 1600, 600));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(20, 30, 151, 51));
        Speed = new QLabel(centralwidget);
        Speed->setObjectName(QString::fromUtf8("Speed"));
        Speed->setGeometry(QRect(180, 30, 81, 51));
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(300, 30, 221, 51));
        Last = new QLabel(centralwidget);
        Last->setObjectName(QString::fromUtf8("Last"));
        Last->setGeometry(QRect(530, 30, 91, 51));
        Count = new QLabel(centralwidget);
        Count->setObjectName(QString::fromUtf8("Count"));
        Count->setGeometry(QRect(770, 30, 141, 51));
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(630, 30, 131, 51));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(20, 100, 251, 51));
        maxSpeed = new QLabel(centralwidget);
        maxSpeed->setObjectName(QString::fromUtf8("maxSpeed"));
        maxSpeed->setGeometry(QRect(280, 100, 81, 51));
        Reset = new QPushButton(centralwidget);
        Reset->setObjectName(QString::fromUtf8("Reset"));
        Reset->setGeometry(QRect(430, 80, 271, 91));
        BoatSpeed->setCentralWidget(centralwidget);
        menubar = new QMenuBar(BoatSpeed);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 1800, 21));
        BoatSpeed->setMenuBar(menubar);
        statusbar = new QStatusBar(BoatSpeed);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        BoatSpeed->setStatusBar(statusbar);

        retranslateUi(BoatSpeed);

        QMetaObject::connectSlotsByName(BoatSpeed);
    } // setupUi

    void retranslateUi(QMainWindow *BoatSpeed)
    {
        BoatSpeed->setWindowTitle(QCoreApplication::translate("BoatSpeed", "BoatSpeed", nullptr));
        Graph->setText(QCoreApplication::translate("BoatSpeed", "Graph", nullptr));
        label_2->setText(QCoreApplication::translate("BoatSpeed", "Speed", nullptr));
        Speed->setText(QCoreApplication::translate("BoatSpeed", "0", nullptr));
        label_4->setText(QCoreApplication::translate("BoatSpeed", "Last Good", nullptr));
        Last->setText(QCoreApplication::translate("BoatSpeed", "0", nullptr));
        Count->setText(QCoreApplication::translate("BoatSpeed", "Count", nullptr));
        label_5->setText(QCoreApplication::translate("BoatSpeed", "Count", nullptr));
        label_3->setText(QCoreApplication::translate("BoatSpeed", "Max Speed", nullptr));
        maxSpeed->setText(QCoreApplication::translate("BoatSpeed", "0", nullptr));
        Reset->setText(QCoreApplication::translate("BoatSpeed", "Reset", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BoatSpeed: public Ui_BoatSpeed {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BOATSPEED_H
