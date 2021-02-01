/********************************************************************************
** Form generated from reading UI file 'ListenPort.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
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
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ListenPort
{
public:
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *portNo;
    QLabel *label_2;
    QTextEdit *CText;
    QVBoxLayout *verticalLayout_2;
    QDialogButtonBox *buttonBox;
    QCheckBox *Enabled;

    void setupUi(QDialog *ListenPort)
    {
        if (ListenPort->objectName().isEmpty())
            ListenPort->setObjectName(QString::fromUtf8("ListenPort"));
        ListenPort->resize(445, 654);
        verticalLayoutWidget = new QWidget(ListenPort);
        verticalLayoutWidget->setObjectName(QString::fromUtf8("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(0, 0, 431, 301));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label = new QLabel(verticalLayoutWidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMinimumSize(QSize(100, 30));
        label->setMaximumSize(QSize(100, 30));

        formLayout->setWidget(2, QFormLayout::LabelRole, label);

        portNo = new QLineEdit(verticalLayoutWidget);
        portNo->setObjectName(QString::fromUtf8("portNo"));
        portNo->setMinimumSize(QSize(0, 30));
        portNo->setMaximumSize(QSize(100, 30));

        formLayout->setWidget(2, QFormLayout::FieldRole, portNo);

        label_2 = new QLabel(verticalLayoutWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMinimumSize(QSize(0, 30));
        label_2->setMaximumSize(QSize(16777215, 30));

        formLayout->setWidget(3, QFormLayout::LabelRole, label_2);

        CText = new QTextEdit(verticalLayoutWidget);
        CText->setObjectName(QString::fromUtf8("CText"));
        CText->setMinimumSize(QSize(0, 150));
        CText->setMaximumSize(QSize(401, 150));

        formLayout->setWidget(3, QFormLayout::FieldRole, CText);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

        formLayout->setLayout(4, QFormLayout::LabelRole, verticalLayout_2);

        buttonBox = new QDialogButtonBox(verticalLayoutWidget);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setMinimumSize(QSize(0, 50));
        buttonBox->setMaximumSize(QSize(16777215, 50));
        buttonBox->setLayoutDirection(Qt::RightToLeft);
        buttonBox->setStandardButtons(QDialogButtonBox::Save);

        formLayout->setWidget(4, QFormLayout::FieldRole, buttonBox);

        Enabled = new QCheckBox(verticalLayoutWidget);
        Enabled->setObjectName(QString::fromUtf8("Enabled"));
        Enabled->setLayoutDirection(Qt::LeftToRight);

        formLayout->setWidget(1, QFormLayout::LabelRole, Enabled);


        verticalLayout->addLayout(formLayout);


        retranslateUi(ListenPort);

        QMetaObject::connectSlotsByName(ListenPort);
    } // setupUi

    void retranslateUi(QDialog *ListenPort)
    {
        ListenPort->setWindowTitle(QCoreApplication::translate("ListenPort", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("ListenPort", "Port", nullptr));
        label_2->setText(QCoreApplication::translate("ListenPort", "CText", nullptr));
        Enabled->setText(QCoreApplication::translate("ListenPort", "Enable Listen    ", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ListenPort: public Ui_ListenPort {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LISTENPORT_H
