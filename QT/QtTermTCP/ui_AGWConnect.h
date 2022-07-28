/********************************************************************************
** Form generated from reading UI file 'AGWConnect.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AGWCONNECT_H
#define UI_AGWCONNECT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AGWConnectDkg
{
public:
    QWidget *verticalLayoutWidget;
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout_2;
    QLabel *label;
    QLineEdit *CallFrom;
    QLabel *label_2;
    QComboBox *CallTo;
    QLabel *label_3;
    QLineEdit *Digis;
    QLabel *label_4;
    QListWidget *RadioPorts;
    QHBoxLayout *hboxLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;

    void setupUi(QDialog *AGWConnectDkg)
    {
        if (AGWConnectDkg->objectName().isEmpty())
            AGWConnectDkg->setObjectName(QString::fromUtf8("AGWConnectDkg"));
        AGWConnectDkg->resize(500, 400);
        verticalLayoutWidget = new QWidget(AGWConnectDkg);
        verticalLayoutWidget->setObjectName(QString::fromUtf8("verticalLayoutWidget"));
        verticalLayoutWidget->setGeometry(QRect(10, 10, 480, 380));
        verticalLayout = new QVBoxLayout(verticalLayoutWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label = new QLabel(verticalLayoutWidget);
        label->setObjectName(QString::fromUtf8("label"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label);

        CallFrom = new QLineEdit(verticalLayoutWidget);
        CallFrom->setObjectName(QString::fromUtf8("CallFrom"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, CallFrom);

        label_2 = new QLabel(verticalLayoutWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_2);

        CallTo = new QComboBox(verticalLayoutWidget);
        CallTo->setObjectName(QString::fromUtf8("CallTo"));
        CallTo->setEditable(true);
        CallTo->setInsertPolicy(QComboBox::NoInsert);

        formLayout_2->setWidget(1, QFormLayout::FieldRole, CallTo);

        label_3 = new QLabel(verticalLayoutWidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        formLayout_2->setWidget(2, QFormLayout::LabelRole, label_3);

        Digis = new QLineEdit(verticalLayoutWidget);
        Digis->setObjectName(QString::fromUtf8("Digis"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, Digis);


        verticalLayout->addLayout(formLayout_2);

        label_4 = new QLabel(verticalLayoutWidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        verticalLayout->addWidget(label_4);

        RadioPorts = new QListWidget(verticalLayoutWidget);
        RadioPorts->setObjectName(QString::fromUtf8("RadioPorts"));

        verticalLayout->addWidget(RadioPorts);

        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(6);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        hboxLayout->setContentsMargins(0, 0, 0, 0);
        okButton = new QPushButton(verticalLayoutWidget);
        okButton->setObjectName(QString::fromUtf8("okButton"));

        hboxLayout->addWidget(okButton);

        cancelButton = new QPushButton(verticalLayoutWidget);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        hboxLayout->addWidget(cancelButton);


        verticalLayout->addLayout(hboxLayout);


        retranslateUi(AGWConnectDkg);
        QObject::connect(okButton, SIGNAL(clicked()), AGWConnectDkg, SLOT(accept()));
        QObject::connect(cancelButton, SIGNAL(clicked()), AGWConnectDkg, SLOT(reject()));

        QMetaObject::connectSlotsByName(AGWConnectDkg);
    } // setupUi

    void retranslateUi(QDialog *AGWConnectDkg)
    {
        AGWConnectDkg->setWindowTitle(QCoreApplication::translate("AGWConnectDkg", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("AGWConnectDkg", "Call From ", nullptr));
        label_2->setText(QCoreApplication::translate("AGWConnectDkg", "Call To ", nullptr));
        label_3->setText(QCoreApplication::translate("AGWConnectDkg", "Digis", nullptr));
        label_4->setText(QCoreApplication::translate("AGWConnectDkg", "Radio Ports", nullptr));
        okButton->setText(QCoreApplication::translate("AGWConnectDkg", "OK", nullptr));
        cancelButton->setText(QCoreApplication::translate("AGWConnectDkg", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AGWConnectDkg: public Ui_AGWConnectDkg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AGWCONNECT_H
