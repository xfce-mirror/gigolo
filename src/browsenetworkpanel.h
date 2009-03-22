/*
 *      browsenetworkpanel.h
 *
 *      Copyright 2009 Enrico Tröger <enrico(at)xfce(dot)org>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */


#ifndef __BROWSENETWORKPANEL_H__
#define __BROWSENETWORKPANEL_H__

G_BEGIN_DECLS

#define GIGOLO_BROWSE_NETWORK_PANEL_TYPE				(gigolo_browse_network_panel_get_type())
#define GIGOLO_BROWSE_NETWORK_PANEL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_BROWSE_NETWORK_PANEL_TYPE, GigoloBrowseNetworkPanel))
#define GIGOLO_BROWSE_NETWORK_PANEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_BROWSE_NETWORK_PANEL_TYPE, GigoloBrowseNetworkPanelClass))
#define IS_GIGOLO_BROWSE_NETWORK_PANEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_BROWSE_NETWORK_PANEL_TYPE))
#define IS_GIGOLO_BROWSE_NETWORK_PANEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_BROWSE_NETWORK_PANEL_TYPE))

typedef struct _GigoloBrowseNetworkPanel			GigoloBrowseNetworkPanel;
typedef struct _GigoloBrowseNetworkPanelClass		GigoloBrowseNetworkPanelClass;

struct _GigoloBrowseNetworkPanel
{
	GtkVBox parent;
};

struct _GigoloBrowseNetworkPanelClass
{
	GtkVBoxClass parent_class;
};


GType		gigolo_browse_network_panel_get_type	(void);
GtkWidget*	gigolo_browse_network_panel_new			(GigoloWindow *parent);

G_END_DECLS

#endif /* __BROWSENETWORKPANEL_H__ */
