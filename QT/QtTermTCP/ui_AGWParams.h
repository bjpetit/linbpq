/********************************************************************************
** Form generated from reading UI file 'AGWParams.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AGWPARAMS_H
#define UI_AGWPARAMS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_AGWParams
{
public:
    QLineEdit *TermCall;
    QGroupBox *groupBox;
    QLineEdit *beaconDest;
    QLineEdit *beaconPath;
    QLineEdit *beaconInterval;
    QLineEdit *beaconPorts;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_5;
    QLabel *label_6;
    QLabel *label_7;
    QLabel *label_11;
    QPlainTextEdit *beaconText;
    QLabel *label_12;
    QGroupBox *groupBox_2;
    QLineEdit *Host;
    QLineEdit *Port;
    QLineEdit *Paclen;
    QLabel *label_8;
    QLabel *label_9;
    QLabel *label_10;
    QLabel *label;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QCheckBox *AGWEnable;
    QLabel *label_13;
    QCheckBox *AGWMonEnable;

    void setupUi(QDialog *AGWParams)
    {
        if (AGWParams->objectName().isEmpty())
            AGWParams->setObjectName(QString::fromUtf8("AGWParams"));
        AGWParams->resize(562, 491);
        TermCall = new QLineEdit(AGWParams);
        TermCall->setObjectName(QString::fromUtf8("TermCall"));
        TermCall->setGeometry(QRect(120, 50, 113, 20));
        groupBox = new QGroupBox(AGWParams);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(12, 83, 537, 215));
        beaconDest = new QLineEdit(groupBox);
        beaconDest->setObjectName(QString::fromUtf8("beaconDest"));
        beaconDest->setGeometry(QRect(110, 24, 113, 20));
        beaconPath = new QLineEdit(groupBox);
        beaconPath->setObjectName(QString::fromUtf8("beaconPath"));
        beaconPath->setGeometry(QRect(110, 54, 113, 20));
        beaconInterval = new QLineEdit(groupBox);
        beaconInterval->setObjectName(QString::fromUtf8("beaconInterval"));
        beaconInterval->setGeometry(QRect(108, 84, 45, 20));
        beaconPorts = new QLineEdit(groupBox);
        beaconPorts->setObjectName(QString::fromUtf8("beaconPorts"));
        beaconPorts->setGeometry(QRect(110, 114, 43, 20));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(14, 22, 75, 21));
        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(14, 52, 75, 21));
        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(14, 82, 75, 21));
        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(14, 112, 75, 21));
        label_6 = new QLabel(groupBox);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(176, 82, 75, 22));
        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(176, 112, 137, 21));
        label_11 = new QLabel(groupBox);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(14, 144, 75, 21));
        beaconText = new QPlainTextEdit(groupBox);
        beaconText->setObjectName(QString::fromUtf8("beaconText"));
        beaconText->setGeometry(QRect(110, 140, 425, 63));
        label_12 = new QLabel(groupBox);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setGeometry(QRect(14, 158, 95, 21));
        groupBox_2 = new QGroupBox(AGWParams);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        groupBox_2->setGeometry(QRect(12, 300, 537, 123));
        Host = new QLineEdit(groupBox_2);
        Host->setObjectName(QString::fromUtf8("Host"));
        Host->setGeometry(QRect(110, 24, 101, 20));
        Port = new QLineEdit(groupBox_2);
        Port->setObjectName(QString::fromUtf8("Port"));
        Port->setGeometry(QRect(110, 54, 47, 20));
        Paclen = new QLineEdit(groupBox_2);
        Paclen->setObjectName(QString::fromUtf8("Paclen"));
        Paclen->setGeometry(QRect(110, 84, 47, 20));
        label_8 = new QLabel(groupBox_2);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(16, 24, 47, 13));
        label_9 = new QLabel(groupBox_2);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(16, 54, 47, 13));
        label_10 = new QLabel(groupBox_2);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setGeometry(QRect(16, 84, 47, 13));
        label = new QLabel(AGWParams);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(18, 48, 111, 21));
        okButton = new QPushButton(AGWParams);
        okButton->setObjectName(QString::fromUtf8("okButton"));
        okButton->setGeometry(QRect(180, 440, 95, 23));
        cancelButton = new QPushButton(AGWParams);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setGeometry(QRect(292, 440, 79, 23));
        AGWEnable = new QCheckBox(AGWParams);
        AGWEnable->setObjectName(QString::fromUtf8("AGWEnable"));
        AGWEnable->setGeometry(QRect(152, 16, 23, 25));
        AGWEnable->setLayoutDirection(Qt::RightToLeft);
        label_13 = new QLabel(AGWParams);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setGeometry(QRect(20, 18, 135, 21));
        AGWMonEnable = new QCheckBox(AGWParams);
        AGWMonEnable->setObjectName(QString::fromUtf8("AGWMonEnable"));
        AGWMonEnable->setGeometry(QRect(255, 18, 216, 21));
        AGWMonEnable->setLayoutDirection(Qt::RightToLeft);

        retranslateUi(AGWParams);
        QObject::connect(cancelButton, SIGNAL(clicked()), AGWParams, SLOT(reject()));
        QObject::connect(okButton, SIGNAL(clicked()), AGWParams, SLOT(accept()));

        QMetaObject::connectSlotsByName(AGWParams);
    } // setupUi

    void retranslateUi(QDialog *AGWParams)
    {
        AGWParams->setWindowTitle(QCoreApplication::translate("AGWParams", "Dialog", nullptr));
        groupBox->setTitle(QCoreApplication::translate("AGWParams", "Beacon Setup", nullptr));
        label_2->setText(QCoreApplication::translate("AGWParams", "Destination", nullptr));
        label_3->setText(QCoreApplication::translate("AGWParams", "Digipeaters", nullptr));
        label_4->setText(QCoreApplication::translate("AGWParams", "Interval", nullptr));
        label_5->setText(QCoreApplication::translate("AGWParams", "Ports", nullptr));
        label_6->setText(QCoreApplication::translate("AGWParams", "Minutes", nullptr));
        label_7->setText(QCoreApplication::translate("AGWParams", "(Separate with commas)", nullptr));
        label_11->setText(QCoreApplication::translate("AGWParams", "Message", nullptr));
        label_12->setText(QCoreApplication::translate("AGWParams", "(max 256 chars)", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("AGWParams", "TNC Setup", nullptr));
        label_8->setText(QCoreApplication::translate("AGWParams", "Host", nullptr));
        label_9->setText(QCoreApplication::translate("AGWParams", "Port", nullptr));
        label_10->setText(QCoreApplication::translate("AGWParams", "Paclen", nullptr));
        label->setText(QCoreApplication::translate("AGWParams", "Terminal Callsign", nullptr));
        okButton->setText(QCoreApplication::translate("AGWParams", "OK", nullptr));
        cancelButton->setText(QCoreApplication::translate("AGWParams", "Cancel", nullptr));
        AGWEnable->setText(QString());
        label_13->setText(QCoreApplication::translate("AGWParams", "Enable AGW Interface", nullptr));
        AGWMonEnable->setText(QCoreApplication::translate("AGWParams", "Enable AGW Monitor Window", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AGWParams: public Ui_AGWParams {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AGWPARAMS_H
