#include <xmmsclient/xmmsclient++.h>

#include <QMainWindow>
#include <QMenu>
#include <QWidget>
#include <QGridLayout>
#include <QPixmap>
#include <QLabel>
#include <QPalette>
#include <QFont>
#include <QHBoxLayout>
#include <QMenu>
#include <QProgressBar>
#include <QProgressDialog>
#include <QErrorMessage>
#include <QTimer>
#include <QIcon>


#include "playerwidget.h"
#include "playerbutton.h"
#include "playlistview.h"
#include "progressframe.h"
#include "filedialog.h"
#include "browsedialog.h"
#include "medialibdialog.h"
#include "preferences.h"
#include "volumebar.h"
#include "textdialog.h"
#include "systemtray.h"
#include "infowindow.h"
#include "minimode.h"
#include "jumptofiledialog.h"


PlayerWidget::PlayerWidget (QWidget *parent, XClient *client) : QMainWindow (parent)
{
	QSettings s;

	m_client = client;

	setWindowTitle ("Esperanza");
	setFocusPolicy (Qt::StrongFocus);
	setAttribute (Qt::WA_DeleteOnClose);

	set_colors ();

	QWidget *main_w = new QWidget (this);
	setCentralWidget (main_w);

	QWidget *dummy = new QWidget (main_w);

	QGridLayout *layout = new QGridLayout (main_w);
	m_playlist = new PlaylistView (this, client);
	layout->addWidget (m_playlist, 1, 0, 1, 3);

	QHBoxLayout *pflay = new QHBoxLayout ();
	pflay->setMargin (0);

	m_pf = new ProgressFrame (this);
	pflay->addWidget (m_pf);
	pflay->setStretchFactor (m_pf, 1);

	PlayerButton *min = new PlayerButton (dummy, ":images/minmax.png");
	connect (min, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (min_pressed ()));
	pflay->addWidget (min);

	layout->addLayout (pflay, 0, 0, 1, 3);

	dummy = new QWidget (main_w);
	QHBoxLayout *hbox = new QHBoxLayout (dummy);

	PlayerButton *plus = new PlayerButton (dummy, ":images/plus.png");
	connect (plus, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (plus_pressed (QMouseEvent *)));

	PlayerButton *minus = new PlayerButton (dummy, ":images/minus.png");
	connect (minus, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (minus_pressed (QMouseEvent *)));

	m_playbutt = new PlayerButton (dummy, ":images/play.png");
	connect (m_playbutt, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (play_pressed ()));

	PlayerButton *back = new PlayerButton (dummy, ":images/back.png");
	connect (back, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (back_pressed ()));

	PlayerButton *fwd = new PlayerButton (dummy, ":images/forward.png");
	connect (fwd, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (fwd_pressed ()));

	PlayerButton *sett = new PlayerButton (dummy, ":images/settings.png");
	connect (sett, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (snett_pressed (QMouseEvent *)));

	/*
	PlayerButton *info = new PlayerButton (dummy, ":images/info.png");
	connect (info, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (info_pressed (QMouseEvent *)));
			 */

	VolumeButton *volume = new VolumeButton (dummy, m_client);

	m_playstop = new PlayerButton (dummy, ":images/playstop.png");
	connect (m_playstop, SIGNAL (clicked (QMouseEvent *)),
			 this, SLOT (playstop_pressed ()));

	hbox->addWidget (back);
	hbox->addWidget (m_playbutt);

	hbox->addWidget (m_playstop);
	if (!s.value ("ui/showstop").toBool ())
		m_playstop->hide ();

	hbox->addWidget (fwd);

	hbox->addStretch (1);

	hbox->addWidget (volume);
	hbox->addWidget (plus);
	hbox->addWidget (minus);
	hbox->addWidget (sett);
	/*
	hbox->addWidget (info);
	*/

	layout->addWidget (dummy, 2, 0, 1, 3);

	layout->setRowStretch (1, 1);
	layout->setRowStretch (0, 0);
	layout->setColumnStretch (1, 1);
	layout->setColumnStretch (2, 1);
	layout->setColumnStretch (0, 0);
	layout->setMargin (1);

	m_status = Xmms::Playback::STOPPED;

	m_current_id = 0;
	connect (client, SIGNAL (gotConnection (XClient *)),
			 this, SLOT (got_connection (XClient *))); 

	resize (s.value ("player/windowsize", QSize (550, 350)).toSize());
	if (s.contains ("player/position"))
		move (s.value ("player/position").toPoint ());

	connect (m_client->cache (), SIGNAL (entryChanged (uint32_t)),
			 this, SLOT (entry_changed (uint32_t)));

	connect (m_client->settings (), SIGNAL (settingsChanged ()),
			 this, SLOT (changed_settings ()));

	/* mac specific code to show menus */
#ifdef Q_WS_MACX
	QMenu *m = menuBar ()->addMenu (tr ("&File"));
	m->addAction (tr ("Preferences"), this, SLOT (open_pref ()));
	m->addAction (tr ("About"), this, SLOT (open_about ()));
	m = menuBar ()->addMenu (tr ("Playlist"));
	m->addAction (tr ("Add local file"), this, SLOT (add_local_file ()));
	m->addAction (tr ("Add local dir"), this, SLOT (add_local_dir ()));
	m->addSeparator ();
	m->addAction (tr ("Add remote file"), this, SLOT (add_remote_file ()));
	m->addSeparator ();
	m->addAction (tr ("Add files from medialib"), this, SLOT (show_medialib ()));
	m = menuBar ()->addMenu (tr ("Help"));
	m->addAction (tr ("Esperanza Help"), this, SLOT (open_short_help ()));
#endif

	// System Tray setup
	if (QSystemTrayIcon::isSystemTrayAvailable ()) {
		m_systray = new SystemTray (this, m_client);
	} else {
		m_systray = NULL;
	}

	/* create the info window */
	/*
	m_info = new InfoWindow (this, m_client);
	m_info->hide ();
	connect (m_playlist, SIGNAL (selectedID (uint32_t)), m_info, SLOT (set_current_id (uint32_t)));
	*/

	m_mini = new MiniMode (this, m_client);
	m_mini->hide ();
	
	/* run it once first time */
	changed_settings ();
}

void
PlayerWidget::closeEvent (QCloseEvent *ev)
{
	qApp->quit ();
}

void
PlayerWidget::set_colors ()
{
}

void
PlayerWidget::min_pressed ()
{
	m_mini->show ();
	hide ();
}

void
PlayerWidget::changed_settings ()
{
	QSettings s;

	set_colors ();

	if (!s.value ("ui/showstop").toBool ())
		m_playstop->hide ();
	else
		m_playstop->show ();

	if (s.value ("ui/reverseplaytime").toBool ())
		m_pf->setReverse (true);
	
	if (m_systray) {
		if ( s.value ("core/systray").toBool ())
			m_systray->show();
		else
			m_systray->hide();
	}

	update ();
}

void
PlayerWidget::moveEvent (QMoveEvent *ev)
{
	QSettings s;
	s.setValue ("player/position", pos ());
}

void
PlayerWidget::resizeEvent (QResizeEvent *ev)
{
	QSettings s;
	s.setValue ("player/windowsize", ev->size ());
}

void
PlayerWidget::keyPressEvent (QKeyEvent *ev)
{
	QSettings s;

	if (ev->modifiers () != Qt::NoModifier) {
		ev->ignore ();
		return;
	}
	
	switch (ev->key ()) {
		case Qt::Key_Backspace:
		case Qt::Key_Delete:
			/* a little hack here */
			m_playlist->set_removed (true);
			remove_selected ();
			break;
		case Qt::Key_S:
			shuffle_pressed ();
			break;
		case Qt::Key_A:
			add_local_file ();
			break;
		case Qt::Key_R:
			add_remote_file ();
			break;
		case Qt::Key_D:
			add_local_dir ();
			break;
		case Qt::Key_C:
			remove_all ();
			break;
		case Qt::Key_M:
			show_medialib ();
			break;
		case Qt::Key_Enter:
		case Qt::Key_Return:
			m_playlist->jump_pos (QModelIndex ());
			break;
		case Qt::Key_Space:
			play_pressed ();
			break;
		case Qt::Key_B:
			fwd_pressed ();
			break;
		case Qt::Key_V:
			back_pressed ();
			break;
		case Qt::Key_P:
			open_pref ();
			break;
		case Qt::Key_Escape:
			if (isHidden ())
				return;

			if (m_systray && s.value ("core/systray").toBool ()) {
				hide ();
			}
			break;
			/*
		case Qt::Key_I:
			info_pressed (NULL);
			break;
			*/
		case Qt::Key_J:
			jump_pressed ();
			break;
		default:
			ev->ignore ();
			break;
	}
}

void
PlayerWidget::jump_pressed ()
{
	JumpToFileDialog d (this, m_playlist->model ());
	if (d.exec ()) {
		QModelIndex idx = d.sel_item ();
		if (idx.isValid ()) {
			m_playlist->setCurrentIndex (idx);
		}
	}
}

void
PlayerWidget::play_pressed ()
{
	if (m_status == Xmms::Playback::PLAYING)
		m_client->playback.pause (&XClient::log);
	else
		m_client->playback.start (&XClient::log);
}

static bool
dummy_uint (const uint32_t &)
{
	return false;
}

void
PlayerWidget::fwd_pressed ()
{
	m_client->playlist.setNextRel (1, &dummy_uint);
	m_client->playback.tickle (&XClient::log);
}

void
PlayerWidget::back_pressed ()
{
	m_client->playlist.setNextRel (-1, &dummy_uint);
	m_client->playback.tickle (&XClient::log);
}

void
PlayerWidget::show_medialib ()
{
	MedialibDialog *mb = new MedialibDialog (this, m_client);
	mb->show ();
}

void
PlayerWidget::plus_pressed (QMouseEvent *ev)
{
	QMenu m;

	m.addAction (tr ("Add local file"), this, SLOT (add_local_file ()));
	m.addAction (tr ("Add local dir"), this, SLOT (add_local_dir ()));
	m.addSeparator ();
	m.addAction (tr ("Add remote file"), this, SLOT (add_remote_file ()));
	m.addSeparator ();
	m.addAction (tr ("Add files from medialib"), this, SLOT (show_medialib ()));

	m.exec (ev->globalPos ());
}

void
PlayerWidget::info_pressed (QMouseEvent *ev)
{
	if (m_info->isVisible ()) {
		m_info->hide ();
	} else {
		m_info->show ();
	}
}

void
PlayerWidget::snett_pressed (QMouseEvent *ev)
{
	QMenu m;
	m.addAction (tr ("Preferences"), this, SLOT (open_pref ()));
	m.addSeparator ();
	m.addAction (tr ("Shuffle"), this, SLOT (shuffle_pressed ()));
	m.addSeparator ();
	m.addAction (tr ("About"), this, SLOT (open_about ()));
	m.addAction (tr ("Keyboard shortcuts"), this, SLOT (open_short_help ()));
//	m.addAction (tr ("Short-cut editor"), this, SLOT (open_sceditor ()));
	/*
	QMenu *pm = m.addMenu (tr ("Playlist Options"));
	pm->addAction (tr ("Shuffle"));
	pm->addAction (tr ("Random"))->setCheckable (true);
	pm->addAction (tr ("Stop after play"))->setCheckable (true);
	*/

	m.exec (ev->globalPos ());
}

void
PlayerWidget::shuffle_pressed ()
{
	m_client->playlist.shuffle (&XClient::log);
}

/*
void
PlayerWidget::open_sceditor ()
{
	ShortCutEditor *sc = new ShortCutEditor (this, m_client);
	sc->show ();
}
*/

void
PlayerWidget::open_about ()
{
	TextDialog dia (this);
	dia.read_file (":text/about.html");
	dia.exec ();
}

void
PlayerWidget::open_short_help ()
{
	TextDialog dia (this);
	dia.read_file (":text/shortcuts.html");
	dia.exec ();
}

void
PlayerWidget::open_pref ()
{
	PreferenceDialog *pd = new PreferenceDialog (this, m_client);
	pd->show ();
}

void
PlayerWidget::add_remote_file ()
{
	BrowseDialog bd (window (), m_client);
	QStringList files = bd.getFiles ();

	for (int i = 0; i < files.count(); i++) {
		m_client->playlist.addUrlEncoded (files.value (i).toStdString (), &XClient::log);
	}

}

void
PlayerWidget::add_local_dir ()
{
	FileDialog fd (this, "playlist_add_dir");
	QString dir = fd.getDirectory ();
	if (!dir.isNull ())
		m_client->playlist.addRecursive (("file://" + dir).toStdString (), &XClient::log);
}

void
PlayerWidget::add_local_file ()
{
	QStringList files;
	FileDialog fd (this, "playlist_add_file");

	files = fd.getFiles ();

	for (int i = 0; i < files.count(); i++) {
		QString s = "file://" + files.at (i);
		m_client->playlist.addUrl (s.toStdString (), &XClient::log);
	}

}

void
PlayerWidget::minus_pressed (QMouseEvent *ev)
{
	QMenu m;

	m.addAction (tr ("Remove selected files"), this, SLOT (remove_selected ()));
	m.addAction (tr ("Clear playlist"), this, SLOT (remove_all ()));

	m.exec (ev->globalPos ());
}

void
PlayerWidget::remove_selected ()
{
	QModelIndexList itm = m_playlist->getSelection ();
	QList<uint32_t> idlist;

	for (int i = 0; i < itm.size (); i++) {
		QModelIndex idx = itm.at (i);
		if (idx.column () != 0)
			continue;

		idlist.append (idx.row ());
	}

	qSort (idlist);
	for (int i = idlist.size () - 1; i > -1; i --) {
		m_client->playlist.remove (idlist.at (i), &XClient::log);
	}
}

void
PlayerWidget::playstop_pressed ()
{
	m_client->playback.stop (&XClient::log);
}

void
PlayerWidget::remove_all ()
{
	m_client->playlist.clear (&XClient::log);
}

void
PlayerWidget::got_connection (XClient *client)
{
	m_client = client;

	client->playback.signalPlaytime (Xmms::bind (&PlayerWidget::handle_playtime, this));
	client->playback.getPlaytime (Xmms::bind (&PlayerWidget::handle_playtime, this));

	client->playback.getStatus (Xmms::bind (&PlayerWidget::handle_status, this));
	client->playback.broadcastStatus (Xmms::bind (&PlayerWidget::handle_status, this));

	client->setDisconnectCallback (boost::bind (&PlayerWidget::handle_disconnect, this));

	client->playback.broadcastCurrentID (Xmms::bind (&PlayerWidget::handle_current_id, this));
	client->playback.currentID (Xmms::bind (&PlayerWidget::handle_current_id, this));

	/* XXX: broken in c++ bindings
	client->stats.broadcastMediainfoReaderStatus (Xmms::bind (&PlayerWidget::handle_index_status, this));
	client->stats.signalMediainfoReaderUnindexed (Xmms::bind (&PlayerWidget::handle_unindexed, this));
	*/
}

void
PlayerWidget::handle_disconnect ()
{
	QErrorMessage *err = new QErrorMessage (this);
	err->showMessage (tr ("Server died. The application will now quit."));
	err->exec ();
	QApplication::quit ();
}

bool
PlayerWidget::handle_status (const Xmms::Playback::Status &st)
{
	m_status = st;

	if (st == Xmms::Playback::STOPPED ||
		st == Xmms::Playback::PAUSED) {
		m_playbutt->setPx (":images/play.png");
		m_mini->update_playbutton (":images/play.png");
	} else {
		m_playbutt->setPx (":images/pause.png");
		m_mini->update_playbutton (":images/pause.png");
	}

	return true;
}

void
PlayerWidget::entry_changed (uint32_t id)
{
	if (id == m_current_id) {
		new_info (m_client->cache ()->get_info (id));
	}
}

void
PlayerWidget::new_info (const QHash<QString,QVariant> &h)
{
	QString s = QString ("%2 - %3")
		.arg(h["artist"].toString ())
		.arg(h["title"].toString ());
	m_pf->setText (s);
	m_mini->setText (s);

	if (h.contains ("duration")) {
		uint32_t dur = h["duration"].toUInt ();
		m_pf->setMaximum (dur / 1000);
		m_pf->setValue (0);

		m_mini->setMaximum (dur / 1000);
		m_mini->setValue (0);
	}
	if (m_systray &&
		m_current_id == h["id"].toUInt () &&
        m_status == Xmms::Playback::PLAYING &&
        h["id"].toUInt () != 0) {
		m_systray->do_notification (tr("Esperanza is now playing:"), s, m_client->cache ()->get_pixmap (m_current_id));
	}
}


bool
PlayerWidget::handle_playtime (const unsigned int &tme)
{
	m_pf->setValue (tme / 1000);
	m_mini->setValue (tme / 1000);
	return true;
}

bool
PlayerWidget::handle_current_id (const unsigned int &id)
{
	m_current_id = id;
	new_info (m_client->cache ()->get_info (id));
	return true;
}


