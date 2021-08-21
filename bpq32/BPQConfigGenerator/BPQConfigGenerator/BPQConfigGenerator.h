#pragma once

#include <QtWidgets/QMainWindow>
#include "QMessageBox"
#include "QComboBox"
#include "ui_BPQConfigGenerator.h"
#include "QDialogButtonBox"

class BPQConfigGenerator : public QMainWindow
{
	Q_OBJECT

public:
	BPQConfigGenerator(QWidget *parent = Q_NULLPTR);

private slots:
	void clickedSlot();

private:
	Ui::BPQConfigGeneratorClass ui;
	void WriteARDOPPort(FILE * fp);
	void WriteUZ7HOPort(FILE * fp);
	void WriteWINMORPort(FILE * fp);
	void WriteVARAPort(FILE * fp);
	void AddTCPKISSPort(FILE * fp);
	void portSetVisible(int i, int v);
	void CreateBasicConfig();
};



class PortDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PortDialog(QWidget *parent = 0);
	~PortDialog();

public:
	QPushButton *okButton;
	QPushButton *cancelButton;

private slots:
	void PortTypeChanged(int Selected);
	void myaccept();
	void myreject();

private:
	QDialogButtonBox *buttonBox;
};
