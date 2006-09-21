#include "playlistview.h"
#include "playlistmodel.h"
#include "xclient.h"
#include "playerwidget.h"

#include <QHeaderView>
#include <QPainter>

PlaylistDelegate::PlaylistDelegate (QObject *parent, PlaylistModel *model) : QItemDelegate (parent)
{
	m_model = model;
}

void
PlaylistDelegate::paint (QPainter *painter,
						 const QStyleOptionViewItem &option,
						 const QModelIndex &index) const
{
	QStyleOptionViewItem o (option);
	if (index.data (PlaylistModel::CurrentEntryRole).toBool ()) {
		QPalette p (o.palette);
		p.setColor (QPalette::Text, QColor (Qt::red));
		p.setColor (QPalette::HighlightedText, QColor (Qt::red));
		o.palette = p;
	}
	if (index.internalId() != -1) {
		o.state |= QStyle::State_Selected;
		QPalette p (o.palette);
		p.setColor (QPalette::Highlight, p.highlight ().color ().light ());
		o.palette = p;
	} 

	QItemDelegate::paint (painter, o, index);
}

PlaylistView::PlaylistView (QWidget *parent, XClient *client) : QTreeView (parent)
{
	m_client = client;

	m_model = new PlaylistModel (this, m_client);
	setModel (m_model);

	setItemDelegate (new PlaylistDelegate (this, m_model));

	setIndentation (0);
	setAlternatingRowColors (true);
	setItemsExpandable (false);
	setRootIsDecorated (false);

	QHeaderView *head = header ();
	head->resizeSection (0, 40);
	head->setResizeMode (0, QHeaderView::Interactive);

	QPalette p (palette ());
	p.setColor (QPalette::AlternateBase, QColor (230, 230, 230));
	setPalette (p);

    setSelectionMode (QAbstractItemView::ExtendedSelection);
    setSelectionBehavior (QAbstractItemView::SelectRows);

    m_selections = new QItemSelectionModel (m_model);
	setSelectionModel (m_selections);

	connect (m_selections, SIGNAL (currentRowChanged (const QModelIndex &, const QModelIndex &)),
			 this, SLOT (item_selected (const QModelIndex &, const QModelIndex &)));

	connect (this, SIGNAL (doubleClicked (const QModelIndex &)),
			 this, SLOT (jump_pos (const QModelIndex &)));

	connect (client, SIGNAL(gotConnection (XClient *)),
			 this, SLOT (got_connection (XClient *))); 

	setIconSize (QSize (75, 75));
}

void
PlaylistView::got_connection (XClient *client)
{
	m_client = client;
	client->playlist.broadcastCurrentPos (Xmms::bind (&PlaylistView::handle_update_pos, this));
	client->playlist.currentPos (Xmms::bind (&PlaylistView::handle_update_pos, this));
}

bool
PlaylistView::handle_update_pos (const uint32_t &pos)
{
	setCurrentIndex (m_model->index (pos, 0));
	return true;
}

void
PlaylistView::item_selected (const QModelIndex &n, const QModelIndex &old)
{
	if (n.internalId () != -1) {
		setCurrentIndex (old);
		return;
	}

	setExpanded (old, false);
	setExpanded (n, true);
}

static bool
dummy_uint (const uint32_t &)
{
	return false;
}

void
PlaylistView::jump_pos (const QModelIndex &idx)
{
	uint32_t row = idx.row ();
	if (idx.internalId () != -1)
		row = idx.parent ().row ();

	m_client->playlist.setNext (row, &dummy_uint);

	PlayerWidget *pw = dynamic_cast<PlayerWidget *> (parent ());
	if (pw->status () != Xmms::Playback::PLAYING)
		m_client->playback.start (&XClient::log);

	m_client->playback.tickle (&XClient::log);
}

QList<uint32_t>
PlaylistView::getSelection ()
{
	QList<uint32_t> ret;
	QModelIndexList lst = m_selections->selectedIndexes ();
	for (int i = 0; i < lst.size (); i++) {
		QModelIndex idx = lst.at (i);
		if (idx.column () != 0)
			continue;

		ret.append (idx.row ());
	}

	return ret;
}

