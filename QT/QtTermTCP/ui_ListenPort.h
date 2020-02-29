/********************************************************************************
** Form generated from reading UI file 'ListenPort.ui'
**
** Created by: Qt User Interface Compiler version 5.12.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LISTENPORT_H
#define UI_LISTENPORT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>

QT_BEGIN_NAMESPACE

class Ui_ListenPort
{
public:
    QLineEdit *portNo;
    QLabel *label;
    QCheckBox *Enabled;
    QDialogButtonBox *buttonBox;
    QLabel *label_2;
    QTextEdit *CText;

    void setupUi(QDialog *ListenPort)
    {
        if (ListenPort->objectName().isEmpty())
            ListenPort->setObjectName(QString::fromUtf8("ListenPort"));
        ListenPort->resize(372, 219);
        portNo = new QLineEdit(ListenPort);
        portNo->setObjectName(QString::fromUtf8("portNo"));
        portNo->setGeometry(QRect(56, 18, 61, 22));
        label = new QLabel(ListenPort);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(22, 18, 31, 21));
        Enabled = new QCheckBox(ListenPort);
        Enabled->setObjectName(QString::fromUtf8("Enabled"));
        Enabled->setGeometry(QRect(11, 46, 105, 20));
        Enabled->setLayoutDirection(Qt::RightToLeft);
        buttonBox = new QDialogButtonBox(ListenPort);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(154, 162, 77, 28));
        buttonBox->setStandardButtons(QDialogButtonBox::Save);
        label_2 = new QLabel(ListenPort);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(22, 70, 47, 21));
        CText = new QTextEdit(ListenPort);
        CText->setObjectName(QString::fromUtf8("CText"));
        CText->setGeometry(QRect(56, 72, 289, 71));

        retranslateUi(ListenPort);

        QMetaObject::connectSlotsByName(ListenPort);
    } // setupUi

    void retranslateUi(QDialog *ListenPort)
    {
        ListenPort->setWindowTitle(QApplication::translate("ListenPort", "Dialog", nullptr));
        label->setText(QApplication::translate("ListenPort", "Port", nullptr));
        Enabled->setText(QApplication::translate("ListenPort", "Enable Listen    ", nullptr));
        label_2->setText(QApplication::translate("ListenPort", "CText", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ListenPort: public Ui_ListenPort {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LISTENPORT_H
