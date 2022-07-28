/********************************************************************************
** Form generated from reading UI file 'ColourConfig.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_COLOURCONFIG_H
#define UI_COLOURCONFIG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ColourDialog
{
public:
    QWidget *layoutWidget;
    QHBoxLayout *hboxLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QPushButton *TermBG;
    QPushButton *InputBG;
    QPushButton *InputColour;
    QPushButton *TermNormal;
    QPushButton *Echoed;
    QPushButton *Warning;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QLabel *label_9;
    QLabel *label_10;
    QPushButton *MonRX;
    QPushButton *MonitorBG;
    QPushButton *MonOther;
    QPushButton *MonTX;
    QLabel *label_5;
    QLabel *label_6;
    QLabel *label_7;
    QLabel *label_8;

    void setupUi(QDialog *ColourDialog)
    {
        if (ColourDialog->objectName().isEmpty())
            ColourDialog->setObjectName(QString::fromUtf8("ColourDialog"));
        ColourDialog->resize(342, 407);
        layoutWidget = new QWidget(ColourDialog);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(66, 350, 199, 33));
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

        TermBG = new QPushButton(ColourDialog);
        TermBG->setObjectName(QString::fromUtf8("TermBG"));
        TermBG->setGeometry(QRect(216, 15, 75, 23));
        TermBG->setMinimumSize(QSize(75, 0));
        TermBG->setMaximumSize(QSize(75, 16777215));
        InputBG = new QPushButton(ColourDialog);
        InputBG->setObjectName(QString::fromUtf8("InputBG"));
        InputBG->setGeometry(QRect(216, 247, 75, 23));
        InputBG->setMinimumSize(QSize(75, 0));
        InputBG->setMaximumSize(QSize(75, 16777215));
        InputColour = new QPushButton(ColourDialog);
        InputColour->setObjectName(QString::fromUtf8("InputColour"));
        InputColour->setGeometry(QRect(216, 276, 75, 23));
        InputColour->setMinimumSize(QSize(75, 0));
        InputColour->setMaximumSize(QSize(75, 16777215));
        TermNormal = new QPushButton(ColourDialog);
        TermNormal->setObjectName(QString::fromUtf8("TermNormal"));
        TermNormal->setGeometry(QRect(216, 44, 75, 23));
        TermNormal->setMinimumSize(QSize(75, 0));
        TermNormal->setMaximumSize(QSize(75, 16777215));
        Echoed = new QPushButton(ColourDialog);
        Echoed->setObjectName(QString::fromUtf8("Echoed"));
        Echoed->setGeometry(QRect(216, 73, 75, 23));
        Echoed->setMinimumSize(QSize(75, 0));
        Echoed->setMaximumSize(QSize(75, 16777215));
        Warning = new QPushButton(ColourDialog);
        Warning->setObjectName(QString::fromUtf8("Warning"));
        Warning->setGeometry(QRect(216, 102, 75, 23));
        Warning->setMinimumSize(QSize(75, 0));
        Warning->setMaximumSize(QSize(75, 16777215));
        label = new QLabel(ColourDialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(21, 16, 187, 16));
        label_2 = new QLabel(ColourDialog);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(21, 45, 185, 16));
        label_3 = new QLabel(ColourDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(21, 74, 191, 16));
        label_4 = new QLabel(ColourDialog);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(21, 103, 185, 16));
        label_9 = new QLabel(ColourDialog);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        label_9->setGeometry(QRect(21, 248, 189, 16));
        label_10 = new QLabel(ColourDialog);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setGeometry(QRect(21, 277, 183, 16));
        MonRX = new QPushButton(ColourDialog);
        MonRX->setObjectName(QString::fromUtf8("MonRX"));
        MonRX->setGeometry(QRect(216, 189, 75, 23));
        MonRX->setMinimumSize(QSize(75, 0));
        MonRX->setMaximumSize(QSize(75, 16777215));
        MonitorBG = new QPushButton(ColourDialog);
        MonitorBG->setObjectName(QString::fromUtf8("MonitorBG"));
        MonitorBG->setGeometry(QRect(216, 131, 75, 23));
        MonitorBG->setMinimumSize(QSize(75, 0));
        MonitorBG->setMaximumSize(QSize(75, 16777215));
        MonOther = new QPushButton(ColourDialog);
        MonOther->setObjectName(QString::fromUtf8("MonOther"));
        MonOther->setGeometry(QRect(216, 218, 75, 23));
        MonOther->setMinimumSize(QSize(75, 0));
        MonOther->setMaximumSize(QSize(75, 16777215));
        MonTX = new QPushButton(ColourDialog);
        MonTX->setObjectName(QString::fromUtf8("MonTX"));
        MonTX->setGeometry(QRect(216, 160, 75, 23));
        MonTX->setMinimumSize(QSize(75, 0));
        MonTX->setMaximumSize(QSize(75, 16777215));
        label_5 = new QLabel(ColourDialog);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(21, 132, 187, 16));
        label_6 = new QLabel(ColourDialog);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(21, 161, 183, 16));
        label_7 = new QLabel(ColourDialog);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setGeometry(QRect(21, 190, 187, 16));
        label_8 = new QLabel(ColourDialog);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setGeometry(QRect(21, 219, 189, 16));

        retranslateUi(ColourDialog);

        QMetaObject::connectSlotsByName(ColourDialog);
    } // setupUi

    void retranslateUi(QDialog *ColourDialog)
    {
        ColourDialog->setWindowTitle(QCoreApplication::translate("ColourDialog", "Colour Selection", nullptr));
        okButton->setText(QCoreApplication::translate("ColourDialog", "OK", nullptr));
        cancelButton->setText(QCoreApplication::translate("ColourDialog", "Cancel", nullptr));
        TermBG->setText(QString());
        InputBG->setText(QString());
        InputColour->setText(QCoreApplication::translate("ColourDialog", "Input", nullptr));
        TermNormal->setText(QCoreApplication::translate("ColourDialog", "Normal", nullptr));
        Echoed->setText(QCoreApplication::translate("ColourDialog", "Echoed", nullptr));
        Warning->setText(QCoreApplication::translate("ColourDialog", "Warning", nullptr));
        label->setText(QCoreApplication::translate("ColourDialog", "Terminal Background    ", nullptr));
        label_2->setText(QCoreApplication::translate("ColourDialog", "Normal Text", nullptr));
        label_3->setText(QCoreApplication::translate("ColourDialog", "Echoed Text", nullptr));
        label_4->setText(QCoreApplication::translate("ColourDialog", "Warning Text", nullptr));
        label_9->setText(QCoreApplication::translate("ColourDialog", "Input Background    ", nullptr));
        label_10->setText(QCoreApplication::translate("ColourDialog", "Input Text", nullptr));
        MonRX->setText(QCoreApplication::translate("ColourDialog", "RX Text", nullptr));
        MonitorBG->setText(QString());
        MonOther->setText(QCoreApplication::translate("ColourDialog", "Other Text", nullptr));
        MonTX->setText(QCoreApplication::translate("ColourDialog", "TX Text", nullptr));
        label_5->setText(QCoreApplication::translate("ColourDialog", "Monitor Background    ", nullptr));
        label_6->setText(QCoreApplication::translate("ColourDialog", "TX Text", nullptr));
        label_7->setText(QCoreApplication::translate("ColourDialog", "RX Text", nullptr));
        label_8->setText(QCoreApplication::translate("ColourDialog", "Other Text", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ColourDialog: public Ui_ColourDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_COLOURCONFIG_H
