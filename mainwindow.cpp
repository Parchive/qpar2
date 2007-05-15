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
#include "mainwindow.h"

MainWindow::MainWindow(int argc, char * argv[]) {
	setupUi(this);
	
	// menu bar
	create_menu();
	
	// tableWidget
	tableWidget->setHorizontalHeaderLabels(QStringList()
		<< tr("Filename") << tr("Progress") << tr("Block"));
	tableWidget->verticalHeader()->hide();
	
	setAcceptDrops(true);
	
	statusBar()->showMessage(tr("Waiting for par2 files..."));
	operation = none;
	status = undef;
	/*nbdone = 0;
	nbfiles = 0;
	avail_blocks = 0;*/
	file_loaded = false;
	
	show();
	
	if (argc == 2) {
		while (QCoreApplication::hasPendingEvents ())
			QCoreApplication::processEvents();
		sourceFileEdit->setText(QDir::convertSeparators(QString::fromAscii(argv[1])));
		preprocess();
	}
}

/*
 * Create the menu bar
 */
void MainWindow::create_menu() {
	QAction * openAction = new QAction(tr("&Open"), this);
	openAction->setShortcut(tr("Ctrl+O"));
	openAction->setStatusTip(tr("Open par2 files for verification"));
	connect(openAction, SIGNAL(triggered()), this, SLOT(on_browseButton_clicked()));
	
	QAction * quitAction = new QAction(tr("&Quit"), this);
	quitAction->setShortcut(tr("Ctrl+Q"));
	quitAction->setStatusTip(tr("Quit QPar2"));
	connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
	
	QAction * aboutAction = new QAction(tr("&About"), this);
    aboutAction->setStatusTip(tr("Show QPar2's About box"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about_dialog()));
	
	QMenu * fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAction);
    fileMenu->addAction(quitAction);
	
	QMenu * helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}

/*
 * Show a message box with some information about QPar2
 */
void MainWindow::about_dialog() {
	QMessageBox::about(this, tr("About QPar2"),
		tr("<h2>QPar2 v0.1</h2>"
		"<p>Copyright &copy; 2007 Mathieu Guilbaud"
		"<p>QPar2 aims at integrating every functionnality of par2cmdline in a Qt GUI."
		"<p>This software is based on the work of <i>parchive</i> "
		"(<a href=\"http://sourceforge.net/projects/parchive\">http://sourceforge.net/projects/parchive</a>), "
		"especially <i>GPar2<i>."
	));
}

/*
 * Resize the table's columns :
 * 40% for the column 1
 * 40% for the column 2
 * 20% for the column 3
 */
void MainWindow::resize_table_column() {
	// if the vertical scrillbar is visible then reduce the width
	int reduce = (tableWidget->verticalScrollBar()->isVisible()) ?
		tableWidget->verticalScrollBar()->width() + 2 : 0;
	int width = (int)((tableWidget->width()-reduce)/10);
	// 4/10 4/10 2/10
	tableWidget->setColumnWidth(0, 4*width);
	tableWidget->setColumnWidth(1, 4*width);
	tableWidget->setColumnWidth(2, 2*width);
}

/*
 * Pop up an "Open file" dialog box 
 */
void MainWindow::on_browseButton_clicked() {
	if (operation != none)
		return; // avoid problems while working
	QString initialName = sourceFileEdit->text();
	if (initialName.isEmpty()) {
		initialName = QDir::homePath();
	}
	QString fileName = QFileDialog::getOpenFileName(this,
			tr("Choose File"), initialName,
			tr("PAR2 files (*.par2)"));
	fileName = QDir::convertSeparators(fileName);
	if (! fileName.isEmpty()) {
		sourceFileEdit->setText(fileName);
		preprocess();
	}
}

/*
 * Init the repairer, plugs the signals for catching
 * repairere informations and scans the headers.
 */
void MainWindow::preprocess() {
	statusBar()->showMessage(tr("Scanning headers..."));
	operation = scanning;
	status = undef;
	file_loaded = true;
	/*nbdone = 0;
	nbfiles = 0;
	avail_blocks = 0;*/
	
	CommandLine * commandline = new CommandLine;
	Result result = eInvalidCommandLineArguments;
	
	// constructs the command line string
	char ** argv;
	char * args [3];
	argv = args;
	args[0] = "par2";
	args[1] = "r";
	char * buffer = strdup(sourceFileEdit->text().toLocal8Bit());
	args[2] = buffer;
	
	// parse the command line
	bool res = commandline->Parse(3, argv);
	
	if (! res) {
		// there was something wrong with the command line
		errors(cmdline_error);
		verifyButton->setEnabled(false);
	}
	else {
		// create the repairer and connect
		// communication signals
		repairer = new LibPar2(commandline);
		repairer->sig_filename.connect(
			sigc::mem_fun(*this, &MainWindow::signal_filename));
		repairer->sig_progress.connect(
			sigc::mem_fun(*this, &MainWindow::signal_progress));
		repairer->sig_headers.connect(
			sigc::mem_fun(*this, &MainWindow::signal_headers));
		repairer->sig_done.connect(
			sigc::mem_fun(*this, &MainWindow::signal_done));
		
		result = repairer->PreProcess();
		update_status(result);
		verifyButton->setEnabled(true);
	}
	free(buffer);
	globalProgressBar->reset();
	operation = none;
}

/*
 * If par2 files are loaded and not already verified
 * then it scan verify the source files.
 */
void MainWindow::on_verifyButton_clicked() {
	if (operation != none)
		return; // avoid problems while working
	if (! file_loaded)
		errors(notloaded_verify);
	else if (status != verifiable)
		errors(already_verified);
	else {
		statusBar()->showMessage(tr("Verifying..."));
		operation = verifying;
		Result result = repairer->Process(false);
		// activate the "repair" button only if needed
		repairButton->setEnabled(result == eRepairPossible);
		update_status(result);
		operation = none;
	}
}

/*
 * If the set of files has already been verified
 * and is repairable then it repairs the damaged files
 */
void MainWindow::on_repairButton_clicked() {
	if (operation != none)
		return; // avoid problems while working
	if (! file_loaded || status == undef)
		errors(notloaded_repair);
	else if (status == complete)
		errors(notnecessary_repair);
	else if (status == unrepairable)
		errors(notpossible_repair);
	else {
		statusBar()->showMessage(tr("Repairing..."));
		operation = repairing;
		Result result = repairer->Process(true);
		update_status(result);
		operation = none;
	}
}

/*
 * For each new file found by libpar2, a signal with the
 * file name is send. We create a new row if the filename
 * is not present.
 */
void MainWindow::signal_filename(std::string filename) {
	int row = tableWidget->rowCount();
	bool exist = false;
	QString fname = QString::fromAscii(filename.c_str());
	
	// if the table is not empty we search for a row
	// that already contains the filename, if we found
	// one then we return
	if (row) {
		QLabel * l;
		for (int i=0; i<row && !exist; i++) {
			l = (QLabel *)(tableWidget->cellWidget(i, 0));
			if (l->text() == fname) {
				exist = true;
			}
		}
	}
	// if the filename is not already present we create the row
	if (!exist) {
		QProgressBar * pb = new QProgressBar();
		pb->setRange(0, 1000);
		tableWidget->insertRow(row);
		tableWidget->setCellWidget(row, 0, new QLabel(fname));
		tableWidget->setCellWidget(row, 1, pb);
		
		resize_table_column();
		tableWidget->scrollToBottom();
	}
	
	// give back the control to events dispatcher during long operation
	QCoreApplication::processEvents();
}

/*
 * Long operations send their progress.
 * Each header files and each data files send a progress
 * relative to their own progress, not a global progress.
 * The argument is a number between 0 and 1000.
 */
void MainWindow::signal_progress(double progress) {
	if (operation == verifying || operation == scanning) {
		int row = tableWidget->rowCount() - 1;
		QProgressBar * progBar = (QProgressBar *)(tableWidget->cellWidget(row, 1));
		progBar->setValue((int)progress);
	}
	else {
		globalProgressBar->setValue((int)progress);
	}
	
	// give back the control to events dispatcher during long operation
	QCoreApplication::processEvents();
}

/*
 * When verifying this signal is send when the processing the file
 * is done. The function signal_filename is send at the start,
 * except if the file is missing.
 */ 
void MainWindow::signal_done(std::string filename, int blocks_available, 
		int blocks_total) {
	// search the filename in the table. If it doesn't exist
	// then it call signal_filename
	QLabel * prev = (QLabel *)(tableWidget->cellWidget(tableWidget->rowCount()-1, 0));
	if (prev->text() != QString::fromAscii(filename.c_str())) {
		signal_filename(filename);
	}
	
	QLabel * lbl = new QLabel(QString::number(blocks_available)
		+ " / " + QString::number(blocks_total));
	lbl->setAlignment(Qt::AlignCenter);
	
	tableWidget->setCellWidget(tableWidget->rowCount()-1, 2, lbl);
		
	// give back the control to events dispatcher during long operation
	QCoreApplication::processEvents();
}

/*
 * The ParHeader is a summary of the par files.
 * It gives information about the set of par files.
 */
void MainWindow::signal_headers(ParHeaders * headers) {
	QString header_setid = tr("Set ID: ");
	QString header_blocksize = tr("Block size: ");
	QString header_sourceblockcount = tr("Data blocks: ");
	QString header_totalsize = tr("Data size: ");
	QString header_recoverablefiles = tr("Recoverable files: ");
	QString header_otherfiles = tr("Other files: ");
	QString header_chunksize = tr("Chunk size: ");
	// set id
	if (headers->setid.size() != 0) {
		header_setid.append(headers->setid.c_str());
	}
	// block size
	if (headers->block_size != -1) {
		double size = headers->block_size;
		if (size < 1024.0)
			header_blocksize.append(QString::number(size) + tr(" bytes"));
		else if (size < 1024.0*1024)
			header_blocksize.append(QString::number(size/1024.0) + tr(" Kbytes"));
		else if (size < 1024.0*1024*1024)
			header_blocksize.append(QString::number(size/(1024.0*1024)) + tr(" Mbytes"));
		else
			header_blocksize.append(QString::number(size/(1024.0*1024*1024)) + tr(" Gbytes"));
	}
	// chunck ?
	if (headers->chunk_size != -1) {
		header_chunksize.append(QString::number(headers->chunk_size));
	}
	// data block
	if (headers->data_blocks != -1) {
		header_sourceblockcount.append(QString::number(headers->data_blocks));
	}
	// header total size
	if (headers->data_size != -1) {
		double size = headers->data_size;
		if (size < 1024.0)
			header_totalsize.append(QString::number(size) + tr(" bytes"));
		else if (size < 1024.0*1024)
			header_totalsize.append(QString::number(size/1024.0) + tr(" Kbytes"));
		else if (size < 1024.0*1024*1024)
			header_totalsize.append(QString::number(size/(1024.0*1024)) + tr(" Mbytes"));
		else
			header_totalsize.append(QString::number(size/(1024.0*1024*1024)) + tr(" Gbytes"));
	}
	// recoverable files
	if (headers->recoverable_files != -1) {
		nbfiles = headers->recoverable_files;
		header_recoverablefiles.append(QString::number(headers->recoverable_files));
	}
	// other files
	if (headers->other_files != -1) {
		header_otherfiles.append(QString::number(headers->other_files));
	}
	lblSetId->setText(header_setid);
	lblBlockSize->setText(header_blocksize);
	lblDataBlock->setText(header_sourceblockcount);
	lblDataSize->setText(header_totalsize);
	lblRecoverable->setText(header_recoverablefiles);
	lblOtherFiles->setText(header_otherfiles);
	lblChunkSize->setText(header_chunksize);
	// give back the control to events dispatcher during long operation
	QCoreApplication::processEvents();
}

/*
 * Display a different message box for each error
 */
void MainWindow::errors(Error error) {
	// display if trying to repair and no archive is loaded
	if (error == notloaded_repair) {
		QMessageBox::warning(this, tr("Unable to repair archive"),
			tr("A recovery set must be loaded before it can be repaired.  Please check that a set has been loaded and try this request again."),
			QMessageBox::Ok);
		statusBar()->showMessage(tr("Repair failed"));
	}
	// display if trying to verify and archive not loaded
	else if (error == notloaded_verify) {
		QMessageBox::warning(this, tr("Unable to repair archive"),
			tr("A recovery set must be loaded before it can be repaired.  Please check that a set has been loaded and try this request again."),
			QMessageBox::Ok);
		statusBar()->showMessage(tr("Verification failed"));
	}
	// display if archive doesn't need repairing
	else if (error == notnecessary_repair) {
		QMessageBox::information(this, tr("Repair not necessary"),
			tr("Verification of the recovery set has found the archive to be complete.  The archive does not need to be repaired."),
			QMessageBox::Ok);
		statusBar()->showMessage(tr("Archive complete"));
	}
	// display if archive already verified
	else if (error == already_verified) {
		QMessageBox::information(this, tr("Verification completed"),
			tr("Verification of the recovery set has already been performed.  You may repair the archive if necessary."),
			QMessageBox::Ok);
		statusBar()->showMessage(tr("Archive already verified"));
	}
	// display if archive unrepairable
	else if (error == notpossible_repair) {
		QMessageBox::critical(this, tr("Repair not possible"),
			tr("Unable to repair the archive.  You do not have enough recovery blocks to repair."),
			QMessageBox::Ok);
		statusBar()->showMessage(tr("Repair not possible"));
	}
	// display if the loaded files is not a PAR2 file
	else if (error == cmdline_error) {
		QMessageBox::critical(this, tr("The recovery file does not exist"),
			tr("The recovery file does not exist. Please check that the file you load is a PAR2 file."),
			QMessageBox::Ok);
		statusBar()->showMessage(tr("The recovery file does not exist"));
	}
}

/*
 * Update the status variable and the text displayed in the status bar.
 * Result is a typedef define in libpar2/par2cmdline.h
 */
void MainWindow::update_status(Result result) {
	switch (result) {
		case eSuccess:
			if (operation == scanning) {
				status = verifiable;
				statusBar()->showMessage(tr("Scan complete"));
			}
			else if (operation == verifying) {
				status = complete;
				statusBar()->showMessage(tr("Verifying complete, no need to repair"));
			}
			else if (operation == repairing) {
				status = complete;
				statusBar()->showMessage(tr("Repairing complete"));
			}
			break;
		case eRepairPossible:
			statusBar()->showMessage(tr("Repair possible"));
			status = repairable;
			break;
		case eRepairNotPossible:
			statusBar()->showMessage(tr("Repair not possible"));
			status = unrepairable;
			break;
		case eInsufficientCriticalData:
			statusBar()->showMessage(tr("Insufficient critical data"));
			status = unrepairable;
			break;
		case eRepairFailed:
			statusBar()->showMessage(tr("Repair failed"));
			status = unrepairable;
			break;
		case eFileIOError:
			statusBar()->showMessage(tr("I/O error"));
			status = unrepairable;
			break;
		case eLogicError:
			statusBar()->showMessage(tr("Internal error"));
			status = unrepairable;
			break;
		case eMemoryError:
			statusBar()->showMessage(tr("Out of memory"));
			status = unrepairable;
			break;
		default:
			break;
	}
}

/*
 * Drag'n'drop support
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasFormat("text/plain"))
		event->acceptProposedAction();
}

/*
 * Drag'n'drop support
 */
void MainWindow::dropEvent(QDropEvent *event) {
	QString filename = event->mimeData()->text();
	// if the filename is like "file://path" we accept it
	QRegExp rx("file://(.*)");
	if (rx.exactMatch(filename)) {
		filename = rx.cap(1);
		if (QFile::exists(filename)) {
			sourceFileEdit->setText(QDir::convertSeparators(filename));
			preprocess();
		}
	}
	event->acceptProposedAction();
}

