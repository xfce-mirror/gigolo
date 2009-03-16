/*
 *      singleinstance.h
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
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


#ifndef __SINGLEINSTANCE_H__
#define __SINGLEINSTANCE_H__

G_BEGIN_DECLS

#define GIGOLO_SINGLE_INSTANCE_TYPE				(gigolo_single_instance_get_type())
#define GIGOLO_SINGLE_INSTANCE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj),\
			GIGOLO_SINGLE_INSTANCE_TYPE, GigoloSingleInstance))
#define GIGOLO_SINGLE_INSTANCE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass),\
			GIGOLO_SINGLE_INSTANCE_TYPE, GigoloSingleInstanceClass))
#define IS_GIGOLO_SINGLE_INSTANCE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj),\
			GIGOLO_SINGLE_INSTANCE_TYPE))
#define IS_GIGOLO_SINGLE_INSTANCE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass),\
			GIGOLO_SINGLE_INSTANCE_TYPE))

typedef struct _GigoloSingleInstance			GigoloSingleInstance;
typedef struct _GigoloSingleInstanceClass		GigoloSingleInstanceClass;

GType					gigolo_single_instance_get_type		(void);
GigoloSingleInstance*	gigolo_single_instance_new			(void);

gboolean				gigolo_single_instance_is_running	(GigoloSingleInstance *gis);
void					gigolo_single_instance_present		(GigoloSingleInstance *gis);

void					gigolo_single_instance_set_parent	(GigoloSingleInstance *gis,
															 GtkWindow *parent);

G_END_DECLS

#endif /* __SINGLEINSTANCE_H__ */
