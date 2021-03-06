#ifndef DLG_CREATEGAME_H
#define DLG_CREATEGAME_H

#include <QDialog>
#include <QMap>
#include "pb/response.pb.h"

class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class TabRoom;

class DlgCreateGame : public QDialog {
	Q_OBJECT
public:
	DlgCreateGame(TabRoom *_room, const QMap<int, QString> &_gameTypes, QWidget *parent = 0);
private slots:
	void actOK();
	void checkResponse(Response::ResponseCode response);
	void spectatorsAllowedChanged(int state);
private:
	TabRoom *room;
	QMap<int, QString> gameTypes;
	QMap<int, QCheckBox *> gameTypeCheckBoxes;

	QGroupBox *spectatorsGroupBox;
	QLabel *descriptionLabel, *passwordLabel, *maxPlayersLabel;
	QLineEdit *descriptionEdit, *passwordEdit;
	QSpinBox *maxPlayersEdit;
	QCheckBox *onlyBuddiesCheckBox, *onlyRegisteredCheckBox;
	QCheckBox *spectatorsAllowedCheckBox, *spectatorsNeedPasswordCheckBox, *spectatorsCanTalkCheckBox, *spectatorsSeeEverythingCheckBox;
	QPushButton *okButton, *cancelButton;
};

#endif
