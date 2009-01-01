#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# WAF build script
#
# Copyright 2008-2009 Enrico Tröger <enrico(at)xfce(dot)org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$



import Build, Configure, Options, Runner, Task, Utils
import sys, os, subprocess, shutil


APPNAME = 'sion'
VERSION = '0.0.1'

srcdir = '.'
blddir = '_build_'


sources = [ 'src/main.c', 'src/compat.c', 'src/window.c', 'src/bookmark.c', 'src/settings.c',
			'src/menubuttonaction.c', 'src/passworddialog.c', 'src/bookmarkdialog.c',
			'src/bookmarkeditdialog.c', 'src/preferencesdialog.c', 'src/backendgvfs.c',
			'src/common.c' ]



def configure(conf):
	conf.check_tool('compiler_cc intltool misc gnu_dirs')

	conf.check_cfg(package='gtk+-2.0', atleast_version='2.12.0', uselib_store='GTK', mandatory=True)
	conf.check_cfg(package='gtk+-2.0', args='--cflags --libs', uselib_store='GTK')
	conf.check_cfg(package='gio-2.0', atleast_version='2.16.0', uselib_store='GIO', mandatory=True)
	conf.check_cfg(package='gio-2.0', args='--cflags --libs', uselib_store='GIO')

	gtk_version = conf.check_cfg(modversion='gtk+-2.0', uselib_store='GTK')
	gio_version = conf.check_cfg(modversion='gio-2.0', uselib_store='GIO')

	conf.define('GETTEXT_PACKAGE', APPNAME, 1)
	conf.define('PACKAGE', APPNAME, 1)
	conf.define('VERSION', VERSION, 1)

	conf.write_config_header('config.h')

	# debug flags
	if Options.options.debug:
		conf.env.append_value('CCFLAGS', '-g -O0 -DDEBUG -DGSEAL_ENABLE -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -D_FORTIFY_SOURCE=2 -fno-common -Waggregate-return -Wcast-align -Wdeclaration-after-statement -Wextra -Wfloat-equal -Wformat=2 -Wformat-nonliteral -Wformat-security -Wformat-security -Winit-self -Wmissing-declarations -Wmissing-field-initializers -Wmissing-format-attribute -Wmissing-include-dirs -Wmissing-noreturn -Wnested-externs -Wpointer-arith -Wredundant-decls -Wshadow -Wsign-compare -Wundef -Wwrite-strings')

	Utils.pprint('BLUE', 'Summary:')
	print_message(conf, 'Install sion ' + VERSION + ' in', conf.env['PREFIX'])
	print_message(conf, 'Using GTK version', gtk_version or 'Unknown')
	print_message(conf, 'Using GIO version', gio_version or 'Unknown')
	print_message(conf, 'Compiling with debugging support', Options.options.debug and 'yes' or 'no')


def set_options(opt):
	opt.tool_options('compiler_cc')
	opt.tool_options('intltool')
	opt.tool_options('gnu_dirs')

	# Features
	opt.add_option('--enable-debug', action='store_true', default=False,
		help='enable debug mode [default: No]', dest='debug')
	opt.add_option('--update-po', action='store_true', default=False,
		help='update the message catalogs for translation', dest='update_po')


def build(bld):
	obj = bld.new_task_gen('cc', 'program')
	obj.name		= 'sion'
	obj.target		= 'sion'
	obj.source		= sources
	obj.includes	= '.'
	obj.uselib		= 'GTK GIO'

	# Translations
	obj			= bld.new_task_gen('intltool_po')
	obj.podir	= 'po'
	obj.appname	= 'sion'

	# sion.desktop
	obj					= bld.new_task_gen('intltool_in')
	obj.source			= 'sion.desktop.in'
	obj.install_path	= '${DATADIR}/applications'
	obj.flags			= '-d'

	# sion.1
	obj					= bld.new_task_gen('subst')
	obj.source			= 'sion.1.in'
	obj.target			= 'sion.1'
	obj.dict			= { 'VERSION' : VERSION }
	obj.install_path	= 0
	bld.install_files('${MANDIR}/man1', 'sion.1')

	# Docs
	bld.install_files('${DOCDIR}', 'AUTHORS ChangeLog COPYING README NEWS TODO')


def dist():
	import md5
	from Scripting import dist, excludes
	excludes.remove('Makefile')
	excludes.append('sion-%s.tar.bz2.sig' % VERSION)
	filename = dist(APPNAME, VERSION)
	f = file(filename,'rb')
	m = md5.md5()
	readBytes = 100000
	while (readBytes):
		readString = f.read(readBytes)
		m.update(readString)
		readBytes = len(readString)
	f.close()
	launch('gpg --detach-sign --digest-algo SHA512 %s' % filename, 'Signing %s' % filename)
	print 'MD5 sum:', filename, m.hexdigest()
	sys.exit(0)


def shutdown():
	if Options.options.update_po:
		os.chdir('%s/po' % srcdir)
		try:
			try:
				size_old = os.stat('sion.pot').st_size
			except:
				size_old = 0
			subprocess.call(['intltool-update', '--pot'])
			size_new = os.stat('sion.pot').st_size
			if size_new != size_old:
				Utils.pprint('CYAN', 'Updated POT file.')
				launch('intltool-update -r', 'Updating translations', 'CYAN')
			else:
				Utils.pprint('CYAN', 'POT file is up to date.')
		except:
			Utils.pprint('RED', 'Failed to generate pot file.')
		os.chdir('..')


# Simple function to execute a command and print its exit status
def launch(command, status, success_color='GREEN'):
	ret = 0
	Utils.pprint(success_color, status)
	try:
		ret = subprocess.call(command.split())
	except:
		ret = 1

	if ret != 0:
		Utils.pprint('RED', status + ' failed')

	return ret

def print_message(conf, msg, result, color = 'GREEN'):
	conf.check_message_1(msg)
	conf.check_message_2(result, color)
