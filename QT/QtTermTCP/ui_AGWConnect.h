/********************************************************************************
** Form generated from reading UI file 'AGWConnect.ui'
**
** Created by: Qt User Interface Compiler version 5.12.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AGWCONNECT_H
#define UI_AGWCONNECT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AGWConnectDkg
{
public:
    QWidget *layoutWidget;
    QHBoxLayout *hboxLayout;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QLineEdit *CallFrom;
    QLineEdit *Digis;
    QComboBox *CallTo;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QGroupBox *groupBox;
    QListWidget *RadioPorts;

    void setupUi(QDialog *AGWConnectDkg)
    {
        if (AGWConnectDkg->objectName().isEmpty())
            AGWConnectDkg->setObjectName(QString::fromUtf8("AGWConnectDkg"));
        AGWConnectDkg->resize(369, 378);
        layoutWidget = new QWidget(AGWConnectDkg);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(80, 330, 213, 33));
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

        CallFrom = new QLineEdit(AGWConnectDkg);
        CallFrom->setObjectName(QString::fromUtf8("CallFrom"));
        CallFrom->setGeometry(QRect(104, 24, 113, 20));
        Digis = new QLineEdit(AGWConnectDkg);
        Digis->setObjectName(QString::fromUtf8("Digis"));
        Digis->setGeometry(QRect(104, 82, 113, 20));
        CallTo = new QComboBox(AGWConnectDkg);
        CallTo->setObjectName(QString::fromUtf8("CallTo"));
        CallTo->setGeometry(QRect(104, 50, 111, 22));
        CallTo->setEditable(true);
        CallTo->setInsertPolicy(QComboBox::NoInsert);
        label = new QLabel(AGWConnectDkg);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(10, 22, 65, 23));
        label_2 = new QLabel(AGWConnectDkg);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(10, 50, 65, 23));
        label_3 = new QLabel(AGWConnectDkg);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(10, 80, 65, 23));
        groupBox = new QGroupBox(AGWConnectDkg);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(10, 122, 359, 189));
        RadioPorts = new QListWidget(groupBox);
        RadioPorts->setObjectName(QString::fromUtf8("RadioPorts"));
        RadioPorts->setGeometry(QRect(15, 16, 337, 163));

        retranslateUi(AGWConnectDkg);
        QObject::connect(okButton, SIGNAL(clicked()), AGWConnectDkg, SLOT(accept()));
        QObject::connect(cancelButton, SIGNAL(clicked()), AGWConnectDkg, SLOT(reject()));

        QMetaObject::connectSlotsByName(AGWConnectDkg);
    } // setupUi

    void retranslateUi(QDialog *AGWConnectDkg)
    {
        AGWConnectDkg->setWindowTitle(QApplication::translate("AGWConnectDkg", "Dialog", nullptr));
        okButton->setText(QApplication::translate("AGWConnectDkg", "OK", nullptr));
        cancelButton->setText(QApplication::translate("AGWConnectDkg", "Cancel", nullptr));
        label->setText(QApplication::translate("AGWConnectDkg", "Call From ", nullptr));
        label_2->setText(QApplication::translate("AGWConnectDkg", "Call To ", nullptr));
        label_3->setText(QApplication::translate("AGWConnectDkg", "Digis", nullptr));
        groupBox->setTitle(QApplication::translate("AGWConnectDkg", "Radio Ports", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AGWConnectDkg: public Ui_AGWConnectDkg {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AGWCONNECT_H
