#ifndef TAB_MESSAGE_H
#define TAB_MESSAGE_H

#include "tab.h"

class AbstractClient;
class ChatView;
class QLineEdit;
class Event_UserMessage;
class Response;

class TabMessage : public Tab {
	Q_OBJECT
private:
	AbstractClient *client;
	QString userName;
	bool userOnline;
	
	ChatView *chatView;
	QLineEdit *sayEdit;

	QAction *aLeave;
signals:
	void talkClosing(TabMessage *tab);
private slots:
	void sendMessage();
	void actLeave();
	void messageSent(const Response &response);
public:
	TabMessage(TabSupervisor *_tabSupervisor, AbstractClient *_client, const QString &_ownName, const QString &_userName);
	~TabMessage();
	void retranslateUi();
	void closeRequest();
	QString getUserName() const { return userName; }
	QString getTabText() const { return tr("Talking to %1").arg(userName); }

	void processUserMessageEvent(const Event_UserMessage &event);
	void processUserLeft();
	void processUserJoined();
};

#endif
