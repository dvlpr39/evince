/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; c-indent-level: 8 -*- */
/*
 *  Copyright (C) 2004 Marco Pesenti Gritti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include "ev-document.h"

#include "ev-backend-marshalers.h"
#include "ev-job-queue.h"

static void ev_document_class_init (gpointer g_class);

enum
{
	PAGE_CHANGED,
	SCALE_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
GMutex *ev_doc_mutex = NULL;


#define LOG(x) 
GType
ev_document_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo our_info =
		{
			sizeof (EvDocumentIface),
			NULL,
			NULL,
			(GClassInitFunc)ev_document_class_init
		};

		type = g_type_register_static (G_TYPE_INTERFACE,
					       "EvDocument",
					       &our_info, (GTypeFlags)0);
	}

	return type;
}

GQuark
ev_document_error_quark (void)
{
  static GQuark q = 0;
  if (q == 0)
    q = g_quark_from_static_string ("ev-document-error-quark");

  return q;
}

static void
ev_document_class_init (gpointer g_class)
{
	signals[PAGE_CHANGED] =
		g_signal_new ("page_changed",
			      EV_TYPE_DOCUMENT,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EvDocumentIface, page_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	signals[SCALE_CHANGED] =
		g_signal_new ("scale_changed",
			      EV_TYPE_DOCUMENT,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EvDocumentIface, scale_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	g_object_interface_install_property (g_class,
				g_param_spec_string ("title",
						     "Document Title",
						     "The title of the document",
						     NULL,
						     G_PARAM_READABLE));
}

#define PAGE_CACHE_STRING "ev-page-cache"

EvPageCache *
ev_document_get_page_cache (EvDocument *document)
{
	EvPageCache *page_cache;

	g_return_val_if_fail (EV_IS_DOCUMENT (document), NULL);

	page_cache = g_object_get_data (G_OBJECT (document), PAGE_CACHE_STRING);
	if (page_cache == NULL) {
		page_cache = ev_page_cache_new ();
		_ev_page_cache_set_document (page_cache, document);
		g_object_set_data_full (G_OBJECT (document), PAGE_CACHE_STRING, page_cache, g_object_unref);
	}

	return page_cache;
}

GMutex *
ev_document_get_doc_mutex (void)
{
	if (ev_doc_mutex == NULL) {
		ev_doc_mutex = g_mutex_new ();
	}
	return ev_doc_mutex;
}


gboolean
ev_document_load (EvDocument  *document,
		  const char  *uri,
		  GError     **error)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);
	gboolean retval;
	LOG ("ev_document_load");
	retval = iface->load (document, uri, error);
	/* Call this to make the initial cached copy */
	ev_document_get_page_cache (document);
	return retval;
}

gboolean
ev_document_save (EvDocument  *document,
		  const char  *uri,
		  GError     **error)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);
	gboolean retval;

	LOG ("ev_document_save");
	retval = iface->save (document, uri, error);

	return retval;
}

char *
ev_document_get_title (EvDocument  *document)
{
	char *title;

	LOG ("ev_document_get_title");
	g_object_get (document, "title", &title, NULL);

	return title;
}

int
ev_document_get_n_pages (EvDocument  *document)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);
	gint retval;

	LOG ("ev_document_get_n_pages");
	retval = iface->get_n_pages (document);

	return retval;
}

void
ev_document_set_page (EvDocument  *document,
		      int          page)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);

	LOG ("ev_document_set_page");
	iface->set_page (document, page);
}

int
ev_document_get_page (EvDocument *document)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);
	int retval;

	LOG ("ev_document_get_page");
	retval = iface->get_page (document);

	return retval;
}

void
ev_document_set_target (EvDocument  *document,
			GdkDrawable *target)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);

	LOG ("ev_document_set_target");
	iface->set_target (document, target);
}

void
ev_document_set_scale (EvDocument   *document,
		       double        scale)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);

	LOG ("ev_document_set_scale");
	iface->set_scale (document, scale);
}

void
ev_document_set_page_offset (EvDocument  *document,
			     int          x,
			     int          y)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);

	LOG ("ev_document_set_page_offset");
	iface->set_page_offset (document, x, y);
}

void
ev_document_get_page_size   (EvDocument   *document,
			     int           page,
			     int          *width,
			     int          *height)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);

	LOG ("ev_document_get_page_size");
	iface->get_page_size (document, page, width, height);
}

char *
ev_document_get_text (EvDocument   *document,
		      GdkRectangle *rect)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);
	char *retval;

	LOG ("ev_document_get_text");
	retval = iface->get_text (document, rect);

	return retval;
}

EvLink *
ev_document_get_link (EvDocument   *document,
		      int           x,
		      int	    y)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);
	EvLink *retval;

	LOG ("ev_document_get_link");
	retval = iface->get_link (document, x, y);

	return retval;
}

void
ev_document_render (EvDocument  *document,
		    int          clip_x,
		    int          clip_y,
		    int          clip_width,
		    int          clip_height)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);

	LOG ("ev_document_render");
	iface->render (document, clip_x, clip_y, clip_width, clip_height);
}


GdkPixbuf *
ev_document_render_pixbuf (EvDocument *document)
{
	EvDocumentIface *iface = EV_DOCUMENT_GET_IFACE (document);
	GdkPixbuf *retval;

	LOG ("ev_document_render_pixbuf");
	g_assert (iface->render_pixbuf);

	retval = iface->render_pixbuf (document);

	return retval;
}


void
ev_document_page_changed (EvDocument *document)
{
	g_signal_emit (G_OBJECT (document), signals[PAGE_CHANGED], 0);
}

void
ev_document_scale_changed (EvDocument *document)
{
	g_signal_emit (G_OBJECT (document), signals[SCALE_CHANGED], 0);
}
