/********************************************************************************
** Form generated from reading UI file 'KISSConfig.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_KISSCONFIG_H
#define UI_KISSCONFIG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_KISSDialog
{
public:
    QWidget *layoutWidget;
    QHBoxLayout *hboxLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QGroupBox *TNCInfo_3;
    QLineEdit *Host;
    QLineEdit *Port;
    QLabel *label_20;
    QLabel *label_21;
    QLineEdit *Port_2;
    QLabel *label_24;
    QGroupBox *groupBox_5;
    QComboBox *SerialPort;
    QLabel *label_9;
    QLabel *label_23;
    QLineEdit *Speed;
    QLabel *label_14;
    QCheckBox *KISSEnable;
    QLabel *label_22;
    QLineEdit *MYCALL;
    QLabel *label_25;
    QLineEdit *Paclen;
    QLabel *label_26;
    QLineEdit *Maxframe;
    QLabel *label_27;
    QLineEdit *Frack;
    QLabel *label_28;
    QLineEdit *Retries;

    void setupUi(QDialog *KISSDialog)
    {
        if (KISSDialog->objectName().isEmpty())
            KISSDialog->setObjectName(QString::fromUtf8("KISSDialog"));
        KISSDialog->resize(432, 286);
        layoutWidget = new QWidget(KISSDialog);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(40, 238, 351, 33));
        hboxLayout = new QHBoxLayout(layoutWidget);
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        okButton = new QPushButton(layoutWidget);
        okButton->setObjectName(QString::fromUtf8("okButton"));

        hboxLayout->addWidget(okButton);

        cancelButton = new QPushButton(layoutWidget);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        hboxLayout->addWidget(cancelButton);

        TNCInfo_3 = new QGroupBox(KISSDialog);
        TNCInfo_3->setObjectName(QString::fromUtf8("TNCInfo_3"));
        TNCInfo_3->setGeometry(QRect(10, 96, 401, 57));
        Host = new QLineEdit(TNCInfo_3);
        Host->setObjectName(QString::fromUtf8("Host"));
        Host->setGeometry(QRect(90, 20, 111, 22));
        Port = new QLineEdit(TNCInfo_3);
        Port->setObjectName(QString::fromUtf8("Port"));
        Port->setGeometry(QRect(330, 21, 47, 22));
        label_20 = new QLabel(TNCInfo_3);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setGeometry(QRect(16, 24, 47, 13));
        label_21 = new QLabel(TNCInfo_3);
        label_21->setObjectName(QString::fromUtf8("label_21"));
        label_21->setGeometry(QRect(280, 24, 47, 13));
        Port_2 = new QLineEdit(TNCInfo_3);
        Port_2->setObjectName(QString::fromUtf8("Port_2"));
        Port_2->setGeometry(QRect(85, 63, 47, 22));
        label_24 = new QLabel(TNCInfo_3);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        label_24->setGeometry(QRect(20, 66, 47, 13));
        groupBox_5 = new QGroupBox(KISSDialog);
        groupBox_5->setObjectName(QString::fromUtf8("groupBox_5"));
        groupBox_5->setGeometry(QRect(0, 46, 411, 51));
        SerialPort = new QComboBox(groupBox_5);
        SerialPort->setObjectName(QString::fromUtf8("SerialPort"));
        SerialPort->setGeometry(QRect(150, 20, 111, 22));
        label_9 = new QLabel(groupBox_5);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(20, 23, 131, 16));
        label_23 = new QLabel(groupBox_5);
        label_23->setObjectName(QString::fromUtf8("label_23"));
        label_23->setGeometry(QRect(280, 20, 47, 22));
        Speed = new QLineEdit(groupBox_5);
        Speed->setObjectName(QString::fromUtf8("Speed"));
        Speed->setGeometry(QRect(335, 21, 51, 22));
        label_14 = new QLabel(KISSDialog);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setGeometry(QRect(10, 12, 161, 21));
        KISSEnable = new QCheckBox(KISSDialog);
        KISSEnable->setObjectName(QString::fromUtf8("KISSEnable"));
        KISSEnable->setGeometry(QRect(150, 10, 23, 25));
        KISSEnable->setLayoutDirection(Qt::RightToLeft);
        label_22 = new QLabel(KISSDialog);
        label_22->setObjectName(QString::fromUtf8("label_22"));
        label_22->setGeometry(QRect(230, 13, 61, 17));
        MYCALL = new QLineEdit(KISSDialog);
        MYCALL->setObjectName(QString::fromUtf8("MYCALL"));
        MYCALL->setGeometry(QRect(300, 10, 91, 22));
        label_25 = new QLabel(KISSDialog);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setGeometry(QRect(25, 168, 47, 13));
        Paclen = new QLineEdit(KISSDialog);
        Paclen->setObjectName(QString::fromUtf8("Paclen"));
        Paclen->setGeometry(QRect(100, 165, 47, 22));
        label_26 = new QLabel(KISSDialog);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        label_26->setGeometry(QRect(265, 168, 66, 16));
        Maxframe = new QLineEdit(KISSDialog);
        Maxframe->setObjectName(QString::fromUtf8("Maxframe"));
        Maxframe->setGeometry(QRect(340, 165, 47, 22));
        label_27 = new QLabel(KISSDialog);
        label_27->setObjectName(QString::fromUtf8("label_27"));
        label_27->setGeometry(QRect(25, 202, 47, 13));
        Frack = new QLineEdit(KISSDialog);
        Frack->setObjectName(QString::fromUtf8("Frack"));
        Frack->setGeometry(QRect(100, 199, 36, 22));
        label_28 = new QLabel(KISSDialog);
        label_28->setObjectName(QString::fromUtf8("label_28"));
        label_28->setGeometry(QRect(265, 201, 47, 13));
        Retries = new QLineEdit(KISSDialog);
        Retries->setObjectName(QString::fromUtf8("Retries"));
        Retries->setGeometry(QRect(340, 198, 36, 22));

        retranslateUi(KISSDialog);
        QObject::connect(okButton, SIGNAL(clicked()), KISSDialog, SLOT(accept()));
        QObject::connect(cancelButton, SIGNAL(clicked()), KISSDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(KISSDialog);
    } // setupUi

    void retranslateUi(QDialog *KISSDialog)
    {
        KISSDialog->setWindowTitle(QCoreApplication::translate("KISSDialog", "KISS Configuration", nullptr));
        okButton->setText(QCoreApplication::translate("KISSDialog", "OK", nullptr));
        cancelButton->setText(QCoreApplication::translate("KISSDialog", "Cancel", nullptr));
        TNCInfo_3->setTitle(QCoreApplication::translate("KISSDialog", "TCP  Setup", nullptr));
        label_20->setText(QCoreApplication::translate("KISSDialog", "Host", nullptr));
        label_21->setText(QCoreApplication::translate("KISSDialog", "Port", nullptr));
        label_24->setText(QCoreApplication::translate("KISSDialog", "Port", nullptr));
        groupBox_5->setTitle(QCoreApplication::translate("KISSDialog", "Serial TNC", nullptr));
        label_9->setText(QCoreApplication::translate("KISSDialog", "Select Device", nullptr));
        label_23->setText(QCoreApplication::translate("KISSDialog", "Speed", nullptr));
        Speed->setText(QCoreApplication::translate("KISSDialog", "19200", nullptr));
        label_14->setText(QCoreApplication::translate("KISSDialog", "Enable KISS Interface", nullptr));
        KISSEnable->setText(QString());
        label_22->setText(QCoreApplication::translate("KISSDialog", "MYCALL", nullptr));
        label_25->setText(QCoreApplication::translate("KISSDialog", "Paclen", nullptr));
        label_26->setText(QCoreApplication::translate("KISSDialog", "MaxFrame", nullptr));
        label_27->setText(QCoreApplication::translate("KISSDialog", "Frack", nullptr));
        label_28->setText(QCoreApplication::translate("KISSDialog", "Retries", nullptr));
    } // retranslateUi

};

namespace Ui {
    class KISSDialog: public Ui_KISSDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_KISSCONFIG_H
