/********************************************************************************
** Form generated from reading UI file 'QtTermTCP.ui'
**
** Created by: Qt User Interface Compiler version 5.14.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QTTERMTCP_H
#define UI_QTTERMTCP_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QtTermTCPClass
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *QtTermTCPClass)
    {
        if (QtTermTCPClass->objectName().isEmpty())
            QtTermTCPClass->setObjectName(QString::fromUtf8("QtTermTCPClass"));
        QtTermTCPClass->resize(781, 698);
        QFont font;
        font.setFamily(QString::fromUtf8("Arial"));
        font.setPointSize(12);
        QtTermTCPClass->setFont(font);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/QtTermTCP/QtTermTCP.ico"), QSize(), QIcon::Normal, QIcon::Off);
        QtTermTCPClass->setWindowIcon(icon);
        centralWidget = new QWidget(QtTermTCPClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(5, 5, 5, 5);
        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));

        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);

        QtTermTCPClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(QtTermTCPClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        QtTermTCPClass->setStatusBar(statusBar);

        retranslateUi(QtTermTCPClass);

        QMetaObject::connectSlotsByName(QtTermTCPClass);
    } // setupUi

    void retranslateUi(QMainWindow *QtTermTCPClass)
    {
        QtTermTCPClass->setWindowTitle(QCoreApplication::translate("QtTermTCPClass", "QtTermTCP", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QtTermTCPClass: public Ui_QtTermTCPClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QTTERMTCP_H
