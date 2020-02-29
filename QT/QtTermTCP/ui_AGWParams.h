/********************************************************************************
** Form generated from reading UI file 'AGWParams.ui'
**
** Created by: Qt User Interface Compiler version 5.12.6
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

    void setupUi(QDialog *AGWParams)
    {
        if (AGWParams->objectName().isEmpty())
            AGWParams->setObjectName(QString::fromUtf8("AGWParams"));
        AGWParams->resize(338, 421);
        TermCall = new QLineEdit(AGWParams);
        TermCall->setObjectName(QString::fromUtf8("TermCall"));
        TermCall->setGeometry(QRect(112, 50, 113, 20));
        groupBox = new QGroupBox(AGWParams);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(12, 83, 315, 153));
        beaconDest = new QLineEdit(groupBox);
        beaconDest->setObjectName(QString::fromUtf8("beaconDest"));
        beaconDest->setGeometry(QRect(100, 24, 113, 20));
        beaconPath = new QLineEdit(groupBox);
        beaconPath->setObjectName(QString::fromUtf8("beaconPath"));
        beaconPath->setGeometry(QRect(100, 54, 113, 20));
        beaconInterval = new QLineEdit(groupBox);
        beaconInterval->setObjectName(QString::fromUtf8("beaconInterval"));
        beaconInterval->setGeometry(QRect(100, 84, 45, 20));
        beaconPorts = new QLineEdit(groupBox);
        beaconPorts->setObjectName(QString::fromUtf8("beaconPorts"));
        beaconPorts->setGeometry(QRect(102, 114, 43, 20));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(16, 22, 75, 21));
        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(16, 52, 75, 21));
        label_4 = new QLabel(groupBox);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(16, 82, 75, 21));
        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(16, 112, 75, 21));
        label_6 = new QLabel(groupBox);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(176, 82, 75, 21));
        label_7 = new QLabel(groupBox);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(176, 112, 137, 21));
        groupBox_2 = new QGroupBox(AGWParams);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        groupBox_2->setGeometry(QRect(12, 244, 315, 123));
        Host = new QLineEdit(groupBox_2);
        Host->setObjectName(QString::fromUtf8("Host"));
        Host->setGeometry(QRect(100, 24, 101, 20));
        Port = new QLineEdit(groupBox_2);
        Port->setObjectName(QString::fromUtf8("Port"));
        Port->setGeometry(QRect(100, 54, 47, 20));
        Paclen = new QLineEdit(groupBox_2);
        Paclen->setObjectName(QString::fromUtf8("Paclen"));
        Paclen->setGeometry(QRect(100, 84, 47, 20));
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
        label->setGeometry(QRect(20, 48, 91, 21));
        okButton = new QPushButton(AGWParams);
        okButton->setObjectName(QString::fromUtf8("okButton"));
        okButton->setGeometry(QRect(72, 384, 95, 23));
        cancelButton = new QPushButton(AGWParams);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setGeometry(QRect(184, 384, 79, 23));
        AGWEnable = new QCheckBox(AGWParams);
        AGWEnable->setObjectName(QString::fromUtf8("AGWEnable"));
        AGWEnable->setGeometry(QRect(18, 16, 141, 25));
        AGWEnable->setLayoutDirection(Qt::RightToLeft);

        retranslateUi(AGWParams);
        QObject::connect(cancelButton, SIGNAL(clicked()), AGWParams, SLOT(reject()));
        QObject::connect(okButton, SIGNAL(clicked()), AGWParams, SLOT(accept()));

        QMetaObject::connectSlotsByName(AGWParams);
    } // setupUi

    void retranslateUi(QDialog *AGWParams)
    {
        AGWParams->setWindowTitle(QApplication::translate("AGWParams", "Dialog", nullptr));
        groupBox->setTitle(QApplication::translate("AGWParams", "Beacon Setup", nullptr));
        label_2->setText(QApplication::translate("AGWParams", "Destination", nullptr));
        label_3->setText(QApplication::translate("AGWParams", "Digipeaters", nullptr));
        label_4->setText(QApplication::translate("AGWParams", "Interval", nullptr));
        label_5->setText(QApplication::translate("AGWParams", "Ports", nullptr));
        label_6->setText(QApplication::translate("AGWParams", "Minutes", nullptr));
        label_7->setText(QApplication::translate("AGWParams", "(Separate with commas)", nullptr));
        groupBox_2->setTitle(QApplication::translate("AGWParams", "TNC Setup", nullptr));
        label_8->setText(QApplication::translate("AGWParams", "Host", nullptr));
        label_9->setText(QApplication::translate("AGWParams", "Port", nullptr));
        label_10->setText(QApplication::translate("AGWParams", "Paclen", nullptr));
        label->setText(QApplication::translate("AGWParams", "Terminal Callsign", nullptr));
        okButton->setText(QApplication::translate("AGWParams", "OK", nullptr));
        cancelButton->setText(QApplication::translate("AGWParams", "Cancel", nullptr));
        AGWEnable->setText(QApplication::translate("AGWParams", "Enable AGW Interface    ", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AGWParams: public Ui_AGWParams {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AGWPARAMS_H
