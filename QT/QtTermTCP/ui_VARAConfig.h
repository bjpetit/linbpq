/********************************************************************************
** Form generated from reading UI file 'VARAConfig.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VARACONFIG_H
#define UI_VARACONFIG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QWidget *layoutWidget;
    QHBoxLayout *hboxLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QGroupBox *TNCInfo;
    QLineEdit *Host;
    QLineEdit *Port;
    QLineEdit *Path;
    QLabel *label_11;
    QLabel *label_12;
    QLabel *label_13;
    QGroupBox *groupBox_3;
    QComboBox *PTTPort;
    QLabel *label_6;
    QLabel *PTTOnLab;
    QLineEdit *PTTOn;
    QLineEdit *PTTOff;
    QLabel *PTTOffLab;
    QLabel *GPIOLab;
    QLineEdit *GPIOLeft;
    QLineEdit *GPIORight;
    QLabel *GPIOLab2;
    QLabel *CATLabel;
    QLineEdit *CATSpeed;
    QRadioButton *RTSDTR;
    QRadioButton *CAT;
    QLineEdit *VIDPID;
    QLabel *CM108Label;
    QRadioButton *CATHex;
    QRadioButton *CATText;
    QLineEdit *TermCall;
    QLabel *label;
    QLabel *label_14;
    QCheckBox *VARAEnable;
    QGroupBox *VARAMode;
    QRadioButton *VARAHF;
    QRadioButton *VARAFM;
    QRadioButton *VARASAT;
    QGroupBox *HFMode;
    QRadioButton *VARA500;
    QRadioButton *VARA2300;
    QRadioButton *VARA2750;
    QButtonGroup *buttonGroup;
    QButtonGroup *buttonGroup_2;
    QButtonGroup *buttonGroup_3;
    QButtonGroup *buttonGroup_4;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(571, 517);
        layoutWidget = new QWidget(Dialog);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(170, 460, 211, 33));
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

        TNCInfo = new QGroupBox(Dialog);
        TNCInfo->setObjectName(QString::fromUtf8("TNCInfo"));
        TNCInfo->setGeometry(QRect(10, 120, 537, 123));
        Host = new QLineEdit(TNCInfo);
        Host->setObjectName(QString::fromUtf8("Host"));
        Host->setGeometry(QRect(130, 22, 101, 22));
        Port = new QLineEdit(TNCInfo);
        Port->setObjectName(QString::fromUtf8("Port"));
        Port->setGeometry(QRect(130, 54, 47, 22));
        Path = new QLineEdit(TNCInfo);
        Path->setObjectName(QString::fromUtf8("Path"));
        Path->setGeometry(QRect(130, 90, 391, 22));
        label_11 = new QLabel(TNCInfo);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setGeometry(QRect(16, 24, 47, 13));
        label_12 = new QLabel(TNCInfo);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setGeometry(QRect(16, 53, 47, 22));
        label_13 = new QLabel(TNCInfo);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setGeometry(QRect(16, 89, 47, 22));
        groupBox_3 = new QGroupBox(Dialog);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        groupBox_3->setGeometry(QRect(8, 280, 541, 181));
        PTTPort = new QComboBox(groupBox_3);
        PTTPort->setObjectName(QString::fromUtf8("PTTPort"));
        PTTPort->setGeometry(QRect(150, 20, 111, 22));
        label_6 = new QLabel(groupBox_3);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(20, 23, 131, 16));
        PTTOnLab = new QLabel(groupBox_3);
        PTTOnLab->setObjectName(QString::fromUtf8("PTTOnLab"));
        PTTOnLab->setGeometry(QRect(20, 80, 121, 22));
        PTTOn = new QLineEdit(groupBox_3);
        PTTOn->setObjectName(QString::fromUtf8("PTTOn"));
        PTTOn->setGeometry(QRect(150, 83, 247, 22));
        PTTOff = new QLineEdit(groupBox_3);
        PTTOff->setObjectName(QString::fromUtf8("PTTOff"));
        PTTOff->setGeometry(QRect(150, 114, 247, 22));
        PTTOffLab = new QLabel(groupBox_3);
        PTTOffLab->setObjectName(QString::fromUtf8("PTTOffLab"));
        PTTOffLab->setGeometry(QRect(20, 112, 121, 22));
        GPIOLab = new QLabel(groupBox_3);
        GPIOLab->setObjectName(QString::fromUtf8("GPIOLab"));
        GPIOLab->setGeometry(QRect(20, 52, 111, 16));
        GPIOLeft = new QLineEdit(groupBox_3);
        GPIOLeft->setObjectName(QString::fromUtf8("GPIOLeft"));
        GPIOLeft->setGeometry(QRect(150, 50, 25, 20));
        GPIORight = new QLineEdit(groupBox_3);
        GPIORight->setObjectName(QString::fromUtf8("GPIORight"));
        GPIORight->setGeometry(QRect(310, 50, 25, 20));
        GPIOLab2 = new QLabel(groupBox_3);
        GPIOLab2->setObjectName(QString::fromUtf8("GPIOLab2"));
        GPIOLab2->setGeometry(QRect(165, 50, 83, 18));
        CATLabel = new QLabel(groupBox_3);
        CATLabel->setObjectName(QString::fromUtf8("CATLabel"));
        CATLabel->setGeometry(QRect(18, 50, 121, 18));
        CATSpeed = new QLineEdit(groupBox_3);
        CATSpeed->setObjectName(QString::fromUtf8("CATSpeed"));
        CATSpeed->setGeometry(QRect(150, 50, 47, 20));
        RTSDTR = new QRadioButton(groupBox_3);
        buttonGroup_2 = new QButtonGroup(Dialog);
        buttonGroup_2->setObjectName(QString::fromUtf8("buttonGroup_2"));
        buttonGroup_2->addButton(RTSDTR);
        RTSDTR->setObjectName(QString::fromUtf8("RTSDTR"));
        RTSDTR->setGeometry(QRect(290, 22, 131, 18));
        CAT = new QRadioButton(groupBox_3);
        buttonGroup_2->addButton(CAT);
        CAT->setObjectName(QString::fromUtf8("CAT"));
        CAT->setGeometry(QRect(420, 22, 121, 18));
        VIDPID = new QLineEdit(groupBox_3);
        VIDPID->setObjectName(QString::fromUtf8("VIDPID"));
        VIDPID->setGeometry(QRect(150, 50, 121, 20));
        CM108Label = new QLabel(groupBox_3);
        CM108Label->setObjectName(QString::fromUtf8("CM108Label"));
        CM108Label->setGeometry(QRect(20, 50, 111, 18));
        CATHex = new QRadioButton(groupBox_3);
        buttonGroup = new QButtonGroup(Dialog);
        buttonGroup->setObjectName(QString::fromUtf8("buttonGroup"));
        buttonGroup->addButton(CATHex);
        CATHex->setObjectName(QString::fromUtf8("CATHex"));
        CATHex->setGeometry(QRect(290, 50, 131, 18));
        CATText = new QRadioButton(groupBox_3);
        buttonGroup->addButton(CATText);
        CATText->setObjectName(QString::fromUtf8("CATText"));
        CATText->setGeometry(QRect(420, 50, 131, 18));
        TermCall = new QLineEdit(Dialog);
        TermCall->setObjectName(QString::fromUtf8("TermCall"));
        TermCall->setGeometry(QRect(140, 44, 113, 22));
        label = new QLabel(Dialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(16, 42, 131, 22));
        label_14 = new QLabel(Dialog);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setGeometry(QRect(18, 12, 161, 21));
        VARAEnable = new QCheckBox(Dialog);
        VARAEnable->setObjectName(QString::fromUtf8("VARAEnable"));
        VARAEnable->setGeometry(QRect(180, 10, 23, 25));
        VARAEnable->setLayoutDirection(Qt::RightToLeft);
        VARAMode = new QGroupBox(Dialog);
        VARAMode->setObjectName(QString::fromUtf8("VARAMode"));
        VARAMode->setGeometry(QRect(270, 10, 131, 101));
        VARAHF = new QRadioButton(VARAMode);
        buttonGroup_4 = new QButtonGroup(Dialog);
        buttonGroup_4->setObjectName(QString::fromUtf8("buttonGroup_4"));
        buttonGroup_4->addButton(VARAHF);
        VARAHF->setObjectName(QString::fromUtf8("VARAHF"));
        VARAHF->setGeometry(QRect(10, 20, 121, 20));
        VARAHF->setChecked(true);
        VARAFM = new QRadioButton(VARAMode);
        buttonGroup_4->addButton(VARAFM);
        VARAFM->setObjectName(QString::fromUtf8("VARAFM"));
        VARAFM->setGeometry(QRect(10, 43, 121, 20));
        VARASAT = new QRadioButton(VARAMode);
        buttonGroup_4->addButton(VARASAT);
        VARASAT->setObjectName(QString::fromUtf8("VARASAT"));
        VARASAT->setGeometry(QRect(10, 66, 121, 20));
        HFMode = new QGroupBox(Dialog);
        HFMode->setObjectName(QString::fromUtf8("HFMode"));
        HFMode->setGeometry(QRect(410, 10, 131, 101));
        VARA500 = new QRadioButton(HFMode);
        buttonGroup_3 = new QButtonGroup(Dialog);
        buttonGroup_3->setObjectName(QString::fromUtf8("buttonGroup_3"));
        buttonGroup_3->addButton(VARA500);
        VARA500->setObjectName(QString::fromUtf8("VARA500"));
        VARA500->setGeometry(QRect(10, 20, 101, 20));
        VARA2300 = new QRadioButton(HFMode);
        buttonGroup_3->addButton(VARA2300);
        VARA2300->setObjectName(QString::fromUtf8("VARA2300"));
        VARA2300->setGeometry(QRect(10, 43, 101, 20));
        VARA2300->setChecked(true);
        VARA2750 = new QRadioButton(HFMode);
        buttonGroup_3->addButton(VARA2750);
        VARA2750->setObjectName(QString::fromUtf8("VARA2750"));
        VARA2750->setGeometry(QRect(10, 66, 101, 21));

        retranslateUi(Dialog);

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "VARA Configuration", nullptr));
        okButton->setText(QCoreApplication::translate("Dialog", "OK", nullptr));
        cancelButton->setText(QCoreApplication::translate("Dialog", "Cancel", nullptr));
        TNCInfo->setTitle(QCoreApplication::translate("Dialog", "TNC Setup", nullptr));
        label_11->setText(QCoreApplication::translate("Dialog", "Host", nullptr));
        label_12->setText(QCoreApplication::translate("Dialog", "Port", nullptr));
        label_13->setText(QCoreApplication::translate("Dialog", "Path", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("Dialog", "PTT Port", nullptr));
        label_6->setText(QCoreApplication::translate("Dialog", "Select PTT Port", nullptr));
        PTTOnLab->setText(QCoreApplication::translate("Dialog", "PTT On String", nullptr));
        PTTOffLab->setText(QCoreApplication::translate("Dialog", "PTT Off String", nullptr));
        GPIOLab->setText(QCoreApplication::translate("Dialog", "GPIO Pin Left", nullptr));
        GPIOLab2->setText(QCoreApplication::translate("Dialog", "GPIO Pin Right", nullptr));
        CATLabel->setText(QCoreApplication::translate("Dialog", "CAT Port Speed", nullptr));
        RTSDTR->setText(QCoreApplication::translate("Dialog", "RTS/DTR", nullptr));
        CAT->setText(QCoreApplication::translate("Dialog", "CAT", nullptr));
        CM108Label->setText(QCoreApplication::translate("Dialog", "CM108 VID/PID", nullptr));
        CATHex->setText(QCoreApplication::translate("Dialog", "Hex Strings", nullptr));
        CATText->setText(QCoreApplication::translate("Dialog", "Text Strings", nullptr));
        label->setText(QCoreApplication::translate("Dialog", "Terminal Callsign", nullptr));
        label_14->setText(QCoreApplication::translate("Dialog", "Enable VARA Interface", nullptr));
        VARAEnable->setText(QString());
        VARAMode->setTitle(QCoreApplication::translate("Dialog", "VARA Mode", nullptr));
        VARAHF->setText(QCoreApplication::translate("Dialog", "VARA HF", nullptr));
        VARAFM->setText(QCoreApplication::translate("Dialog", "VARA FM", nullptr));
        VARASAT->setText(QCoreApplication::translate("Dialog", "VARA SAT", nullptr));
        HFMode->setTitle(QCoreApplication::translate("Dialog", "VARA HF BW", nullptr));
        VARA500->setText(QCoreApplication::translate("Dialog", "500 Hz", nullptr));
        VARA2300->setText(QCoreApplication::translate("Dialog", "2300 Hz", nullptr));
        VARA2750->setText(QCoreApplication::translate("Dialog", "2750 Hz", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VARACONFIG_H
