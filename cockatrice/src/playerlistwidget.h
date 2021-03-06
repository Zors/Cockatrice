#ifndef PLAYERLISTWIDGET_H
#define PLAYERLISTWIDGET_H

#include <QTreeWidget>
#include <QMap>
#include <QIcon>
#include <QStyledItemDelegate>

class ServerInfo_PlayerProperties;
class TabSupervisor;
class AbstractClient;
class TabGame;

class PlayerListItemDelegate : public QStyledItemDelegate {
public:
	PlayerListItemDelegate(QObject *const parent);
	bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);
};

class PlayerListTWI : public QTreeWidgetItem {
public:
	PlayerListTWI();
	bool operator<(const QTreeWidgetItem &other) const;
};

class PlayerListWidget : public QTreeWidget {
	Q_OBJECT
private:
	PlayerListItemDelegate *itemDelegate;
	QMap<int, QTreeWidgetItem *> players;
	TabSupervisor *tabSupervisor;
	AbstractClient *client;
	TabGame *game;
	QIcon readyIcon, notReadyIcon, concededIcon, playerIcon, spectatorIcon;
	bool gameStarted;
signals:
	void openMessageDialog(const QString &userName, bool focus);
public:
	PlayerListWidget(TabSupervisor *_tabSupervisor, AbstractClient *_client, TabGame *_game, QWidget *parent = 0);
	void retranslateUi();
	void addPlayer(const ServerInfo_PlayerProperties &player);
	void removePlayer(int playerId);
	void setActivePlayer(int playerId);
	void updatePlayerProperties(const ServerInfo_PlayerProperties &prop, int playerId = -1);
	void setGameStarted(bool _gameStarted, bool resuming);
	void showContextMenu(const QPoint &pos, const QModelIndex &index);
};

#endif
