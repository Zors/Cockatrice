#include <QTreeView>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>
#include <QGroupBox>
#include <QHeaderView>
#include <QApplication>
#include <QInputDialog>
#include "tab_deck_storage.h"
#include "remotedecklist_treewidget.h"
#include "abstractclient.h"
#include "decklist.h"
#include "window_deckeditor.h"
#include "settingscache.h"

#include "pending_command.h"
#include "pb/response.pb.h"
#include "pb/response_deck_download.pb.h"
#include "pb/response_deck_upload.pb.h"
#include "pb/command_deck_upload.pb.h"
#include "pb/command_deck_download.pb.h"
#include "pb/command_deck_new_dir.pb.h"
#include "pb/command_deck_del_dir.pb.h"
#include "pb/command_deck_del.pb.h"

TabDeckStorage::TabDeckStorage(TabSupervisor *_tabSupervisor, AbstractClient *_client)
	: Tab(_tabSupervisor), client(_client)
{
	localDirModel = new QFileSystemModel(this);
	localDirModel->setRootPath(settingsCache->getDeckPath());
	
	sortFilter = new QSortFilterProxyModel(this);
	sortFilter->setSourceModel(localDirModel);
	sortFilter->setDynamicSortFilter(true);
	
	localDirView = new QTreeView;
	localDirView->setModel(sortFilter);
	localDirView->setColumnHidden(1, true);
	localDirView->setRootIndex(sortFilter->mapFromSource(localDirModel->index(localDirModel->rootPath(), 0)));
	localDirView->setSortingEnabled(true);
	localDirView->header()->setResizeMode(QHeaderView::ResizeToContents);
	sortFilter->sort(0, Qt::AscendingOrder);
	localDirView->header()->setSortIndicator(0, Qt::AscendingOrder);
	
	leftToolBar = new QToolBar;
	leftToolBar->setOrientation(Qt::Horizontal);
	leftToolBar->setIconSize(QSize(32, 32));
	QHBoxLayout *leftToolBarLayout = new QHBoxLayout;
	leftToolBarLayout->addStretch();
	leftToolBarLayout->addWidget(leftToolBar);
	leftToolBarLayout->addStretch();

	QVBoxLayout *leftVbox = new QVBoxLayout;
	leftVbox->addWidget(localDirView);
	leftVbox->addLayout(leftToolBarLayout);
	leftGroupBox = new QGroupBox;
	leftGroupBox->setLayout(leftVbox);
	
	rightToolBar = new QToolBar;
	rightToolBar->setOrientation(Qt::Horizontal);
	rightToolBar->setIconSize(QSize(32, 32));
	QHBoxLayout *rightToolBarLayout = new QHBoxLayout;
	rightToolBarLayout->addStretch();
	rightToolBarLayout->addWidget(rightToolBar);
	rightToolBarLayout->addStretch();

	serverDirView = new RemoteDeckList_TreeWidget(client);

	QVBoxLayout *rightVbox = new QVBoxLayout;
	rightVbox->addWidget(serverDirView);
	rightVbox->addLayout(rightToolBarLayout);
	rightGroupBox = new QGroupBox;
	rightGroupBox->setLayout(rightVbox);
	
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget(leftGroupBox);
	hbox->addWidget(rightGroupBox);
	
	aOpenLocalDeck = new QAction(this);
	aOpenLocalDeck->setIcon(QIcon(":/resources/pencil.svg"));
	connect(aOpenLocalDeck, SIGNAL(triggered()), this, SLOT(actOpenLocalDeck()));
	aUpload = new QAction(this);
	aUpload->setIcon(QIcon(":/resources/arrow_right_green.svg"));
	connect(aUpload, SIGNAL(triggered()), this, SLOT(actUpload()));
	aOpenRemoteDeck = new QAction(this);
	aOpenRemoteDeck->setIcon(QIcon(":/resources/pencil.svg"));
	connect(aOpenRemoteDeck, SIGNAL(triggered()), this, SLOT(actOpenRemoteDeck()));
	aDownload = new QAction(this);
	aDownload->setIcon(QIcon(":/resources/arrow_left_green.svg"));
	connect(aDownload, SIGNAL(triggered()), this, SLOT(actDownload()));
	aNewFolder = new QAction(this);
	aNewFolder->setIcon(qApp->style()->standardIcon(QStyle::SP_FileDialogNewFolder));
	connect(aNewFolder, SIGNAL(triggered()), this, SLOT(actNewFolder()));
	aDelete = new QAction(this);
	aDelete->setIcon(QIcon(":/resources/remove_row.svg"));
	connect(aDelete, SIGNAL(triggered()), this, SLOT(actDelete()));
	
	leftToolBar->addAction(aOpenLocalDeck);
	leftToolBar->addAction(aUpload);
	rightToolBar->addAction(aOpenRemoteDeck);
	rightToolBar->addAction(aDownload);
	rightToolBar->addAction(aNewFolder);
	rightToolBar->addAction(aDelete);
	
	retranslateUi();
	setLayout(hbox);
}

void TabDeckStorage::retranslateUi()
{
	leftGroupBox->setTitle(tr("Local file system"));
	rightGroupBox->setTitle(tr("Server deck storage"));
	
	aOpenLocalDeck->setText(tr("Open in deck editor"));
	aUpload->setText(tr("Upload deck"));
	aOpenRemoteDeck->setText(tr("Open in deck editor"));
	aDownload->setText(tr("Download deck"));
	aNewFolder->setText(tr("New folder"));
	aDelete->setText(tr("Delete"));
}

void TabDeckStorage::actOpenLocalDeck()
{
	QModelIndex curLeft = sortFilter->mapToSource(localDirView->selectionModel()->currentIndex());
	if (localDirModel->isDir(curLeft))
		return;
	QString filePath = localDirModel->filePath(curLeft);
	DeckList *deck = new DeckList;
	if (!deck->loadFromFile(filePath, DeckList::CockatriceFormat))
		return;
	
	WndDeckEditor *deckEditor = new WndDeckEditor;
	deckEditor->setDeck(deck, filePath, DeckList::CockatriceFormat);
	deckEditor->show();
}

void TabDeckStorage::actUpload()
{
	QModelIndex curLeft = sortFilter->mapToSource(localDirView->selectionModel()->currentIndex());
	if (localDirModel->isDir(curLeft))
		return;
	QString filePath = localDirModel->filePath(curLeft);
	QFile deckFile(filePath);
	QFileInfo deckFileInfo(deckFile);
	DeckList deck;
	if (!deck.loadFromFile(filePath, DeckList::CockatriceFormat))
		return;
	if (deck.getName().isEmpty()) {
		bool ok;
		QString deckName = QInputDialog::getText(this, tr("Enter deck name"), tr("This decklist does not have a name.\nPlease enter a name:"), QLineEdit::Normal, deckFileInfo.completeBaseName(), &ok);
		if (!ok)
			return;
		if (deckName.isEmpty())
			deckName = tr("Unnamed deck");
		deck.setName(deckName);
	}

	QString targetPath;
	RemoteDeckList_TreeModel::Node *curRight = serverDirView->getCurrentItem();
	if (!curRight)
		return;
	if (!dynamic_cast<RemoteDeckList_TreeModel::DirectoryNode *>(curRight))
		curRight = curRight->getParent();
	targetPath = dynamic_cast<RemoteDeckList_TreeModel::DirectoryNode *>(curRight)->getPath();
	
	Command_DeckUpload cmd;
	cmd.set_path(targetPath.toStdString());
	cmd.set_deck_list(deck.writeToString_Native().toStdString());
	
	PendingCommand *pend = client->prepareSessionCommand(cmd);
	connect(pend, SIGNAL(finished(const Response &)), this, SLOT(uploadFinished(const Response &)));
	client->sendCommand(pend);
}

void TabDeckStorage::uploadFinished(const Response &r)
{
	const Response_DeckUpload &resp = r.GetExtension(Response_DeckUpload::ext);
	const Command_DeckUpload &cmd = static_cast<const Command_DeckUpload &>(static_cast<PendingCommand *>(sender())->getCommandContainer().session_command(0).GetExtension(Command_DeckUpload::ext));
	
	serverDirView->addFileToTree(resp.new_file(), serverDirView->getNodeByPath(QString::fromStdString(cmd.path())));
}

void TabDeckStorage::actOpenRemoteDeck()
{
	RemoteDeckList_TreeModel::FileNode *curRight = dynamic_cast<RemoteDeckList_TreeModel::FileNode *>(serverDirView->getCurrentItem());
	if (!curRight)
		return;
	
	Command_DeckDownload cmd;
	cmd.set_deck_id(curRight->getId());
	
	PendingCommand *pend = client->prepareSessionCommand(cmd);
	connect(pend, SIGNAL(finished(const Response &)), this, SLOT(openRemoteDeckFinished(const Response &)));
	client->sendCommand(pend);
}

void TabDeckStorage::openRemoteDeckFinished(const Response &r)
{
	const Response_DeckDownload &resp = r.GetExtension(Response_DeckDownload::ext);
	
	WndDeckEditor *deckEditor = new WndDeckEditor;
	deckEditor->setDeck(new DeckList(QString::fromStdString(resp.deck())));
	deckEditor->show();
}

void TabDeckStorage::actDownload()
{
	QString filePath;
	QModelIndex curLeft = sortFilter->mapToSource(localDirView->selectionModel()->currentIndex());
	if (!curLeft.isValid())
		filePath = localDirModel->rootPath();
	else {
		while (!localDirModel->isDir(curLeft))
			curLeft = curLeft.parent();
		filePath = localDirModel->filePath(curLeft);
	}

	RemoteDeckList_TreeModel::FileNode *curRight = dynamic_cast<RemoteDeckList_TreeModel::FileNode *>(serverDirView->getCurrentItem());
	if (!curRight)
		return;
	filePath += QString("/deck_%1.cod").arg(curRight->getId());
	
	Command_DeckDownload cmd;
	cmd.set_deck_id(curRight->getId());
	
	PendingCommand *pend = client->prepareSessionCommand(cmd);
	pend->setExtraData(filePath);
	connect(pend, SIGNAL(finished(const Response &)), this, SLOT(downloadFinished(const Response &)));
	client->sendCommand(pend);
}

void TabDeckStorage::downloadFinished(const Response &r)
{
	const Response_DeckDownload &resp = r.GetExtension(Response_DeckDownload::ext);
	
	PendingCommand *pend = static_cast<PendingCommand *>(sender());
	QString filePath = pend->getExtraData().toString();
	
	DeckList deck(QString::fromStdString(resp.deck()));
	deck.saveToFile(filePath, DeckList::CockatriceFormat);
}

void TabDeckStorage::actNewFolder()
{
	QString folderName = QInputDialog::getText(this, tr("New folder"), tr("Name of new folder:"));
	if (folderName.isEmpty())
		return;

	QString targetPath;
	RemoteDeckList_TreeModel::Node *curRight = serverDirView->getCurrentItem();
	if (!curRight)
		return;
	if (!dynamic_cast<RemoteDeckList_TreeModel::DirectoryNode *>(curRight))
		curRight = curRight->getParent();
	RemoteDeckList_TreeModel::DirectoryNode *dir = dynamic_cast<RemoteDeckList_TreeModel::DirectoryNode *>(curRight);
	targetPath = dir->getPath();
	
	Command_DeckNewDir cmd;
	cmd.set_path(targetPath.toStdString());
	cmd.set_dir_name(folderName.toStdString());
	
	PendingCommand *pend = client->prepareSessionCommand(cmd);
	connect(pend, SIGNAL(finished(Response::ResponseCode)), this, SLOT(newFolderFinished(Response::ResponseCode)));
	client->sendCommand(pend);
}

void TabDeckStorage::newFolderFinished(Response::ResponseCode resp)
{
	if (resp != Response::RespOk)
		return;
	
	const Command_DeckNewDir &cmd = static_cast<const Command_DeckNewDir &>(static_cast<PendingCommand *>(sender())->getCommandContainer().session_command(0).GetExtension(Command_DeckNewDir::ext));
	serverDirView->addFolderToTree(QString::fromStdString(cmd.dir_name()), serverDirView->getNodeByPath(QString::fromStdString(cmd.path())));
}

void TabDeckStorage::actDelete()
{
	PendingCommand *pend;
	RemoteDeckList_TreeModel::Node *curRight = serverDirView->getCurrentItem();
	if (!curRight)
		return;
	RemoteDeckList_TreeModel::DirectoryNode *dir = dynamic_cast<RemoteDeckList_TreeModel::DirectoryNode *>(curRight);
	if (dir) {
		QString path = dir->getPath();
		if (path.isEmpty())
			return;
		Command_DeckDelDir cmd;
		cmd.set_path(path.toStdString());
		pend = client->prepareSessionCommand(cmd);
		connect(pend, SIGNAL(finished(Response::ResponseCode)), this, SLOT(deleteFolderFinished(Response::ResponseCode)));
	} else {
		Command_DeckDel cmd;
		cmd.set_deck_id(dynamic_cast<RemoteDeckList_TreeModel::FileNode *>(curRight)->getId());
		pend = client->prepareSessionCommand(cmd);
		connect(pend, SIGNAL(finished(Response::ResponseCode)), this, SLOT(deleteDeckFinished(Response::ResponseCode)));
	}
	
	client->sendCommand(pend);
}

void TabDeckStorage::deleteDeckFinished(Response::ResponseCode resp)
{
	if (resp != Response::RespOk)
		return;
	
	const Command_DeckDel &cmd = static_cast<const Command_DeckDel &>(static_cast<PendingCommand *>(sender())->getCommandContainer().session_command(0).GetExtension(Command_DeckDel::ext));
	RemoteDeckList_TreeModel::Node *toDelete = serverDirView->getNodeById(cmd.deck_id());
	if (toDelete)
		serverDirView->removeNode(toDelete);
}

void TabDeckStorage::deleteFolderFinished(Response::ResponseCode resp)
{
	if (resp != Response::RespOk)
		return;
	
	const Command_DeckDelDir &cmd = static_cast<const Command_DeckDelDir &>(static_cast<PendingCommand *>(sender())->getCommandContainer().session_command(0).GetExtension(Command_DeckDelDir::ext));
	RemoteDeckList_TreeModel::Node *toDelete = serverDirView->getNodeByPath(QString::fromStdString(cmd.path()));
	if (toDelete)
		serverDirView->removeNode(toDelete);
}
