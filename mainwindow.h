/*
	QPar2 version 0.1.
	QPar2 aims at integrating every functionnality of par2cmdline in a Qt GUI.
	It depends on libpar2, libsigc++ and libqt4.
	
    Copyright (C) 2007  Mathieu Guilbaud

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <libpar2/par2cmdline.h>
#include <libpar2/libpar2.h>
#include <string.h>
#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QHeaderView>
#include <QScrollBar>
#include <QDragEnterEvent>
#include <QDropEvent>

#include "ui_mainwindow.h"

class MainWindow : public QMainWindow, public Ui::MainWindow {
	Q_OBJECT
public:
	MainWindow(int argc, char * argv[]);
	
private slots:
	void on_verifyButton_clicked();
	void on_browseButton_clicked();
	void on_repairButton_clicked();
	void resize_table_column();
	void about_dialog();

private:
	typedef enum {
		none,
		repairing,
		verifying,
		scanning
	} Operation;
	Operation operation;
	
	typedef enum {
		notloaded_repair,
		notloaded_verify,
		notnecessary_repair,
		notpossible_repair,
		already_verified,
		cmdline_error
	} Error;
	
	typedef enum {
		complete,
		repairable,
		unrepairable,
		verifiable,
		undef
	} Status;
	Status status;
	
	LibPar2 * repairer;
	int nbdone;
	int nbfiles;
	bool file_loaded;
	int avail_blocks;
	
	void create_menu();
	void preprocess();
	void errors(Error error);
	void update_status(Result result);
	
protected:
	void signal_filename(std::string filename);
	void signal_progress(double progress);
	void signal_headers(ParHeaders* headers);
	void signal_done(std::string filename, int blocks_available, 
		int blocks_total);
	void dragEnterEvent(QDragEnterEvent * event);
	void dropEvent(QDropEvent * event);
};

#endif
