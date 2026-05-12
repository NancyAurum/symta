# -*- coding: utf-8 -*-
#Notepad++ PythonScript to list all bookmarks in a Shift+LMB clickable view
#https://community.notepad-plus-plus.org/topic/23128/all-bookmarks-list-view
from __future__ import print_function

import os
import re
import time
from ctypes import (WinDLL)

#-------------------------------------------------------------------------------

class SaveAndRestoreActiveViewAndDocument(object):  # a "context manager" to be used with "with"
    def __init__(self):
        self.view_and_index_tuple = None
    def __enter__(self):
        self.view_and_index_tuple = (notepad.getCurrentView(), notepad.getCurrentDocIndex(notepad.getCurrentView()))
    def __exit__(self, exc_type, exc_value, exc_traceback):
        notepad.activateIndex(*self.view_and_index_tuple)
        return False

#-------------------------------------------------------------------------------

class BIODL(object):

    def __init__(self):
        self.header_line_to_look_for = 'BOOKMARKS IN OPEN DOCS (double-click a bottom-level line to jump to that bookmark and close this window; hold Shift to keep this window open)'
        editor.callback(self.doubleclick_callback, [SCINTILLANOTIFICATION.DOUBLECLICK])

    def is_shift_held(self):
        user32 = WinDLL('user32')
        VK_SHIFT = 0x10
        return (user32.GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0

    def doubleclick_callback(self, args):
        dclick_line_nbr = args['line']
        if dclick_line_nbr > 0:
            header_line = editor.getLine(0).rstrip()
            if header_line == self.header_line_to_look_for:
                editor.setEmptySelection(args['position'])
                dclick_line_contents = editor.getLine(dclick_line_nbr)
                m = re.match(r'\tLine (?P<source_line_nbr>\d+)', dclick_line_contents)
                if m:
                    source_line_nbr = int(m.group('source_line_nbr')) - 1
                    while dclick_line_nbr > 0:
                        line_content = editor.getLine(dclick_line_nbr - 1).rstrip()
                        m = re.match(r'  "(?P<filename>.+?)"(?: in "(?P<dirname>.+?)")?', line_content)
                        if m:
                            filename = m.group('filename')
                            dirname = m.group('dirname')
                            pathname = dirname + filename if dirname else filename
                            if not self.is_shift_held(): notepad.close()
                            notepad.open(pathname)
                            time.sleep(0.1)  # sometimes a slow double-click will end up affecting the file switching to...
                            editor.setFirstVisibleLine(editor.visibleFromDocLine(source_line_nbr) - editor.linesOnScreen() / 2)
                            editor.setEmptySelection(editor.positionFromLine(source_line_nbr))
                            editor.chooseCaretX()  # if up or down key is used from here, stay in column 1
                            break
                        dclick_line_nbr -= 1

    def run(self):
        NPP_BOOKMARK_MARKER_ID_NUMBER = 20  # from N++ source code: MARK_BOOKMARK = 24;
        NPP_BOOKMARK_MARKER_MASK = 1 << NPP_BOOKMARK_MARKER_ID_NUMBER
        line_nbr_and_content_tup_by_path_dict = {}
        with SaveAndRestoreActiveViewAndDocument():
            for (pathname, buffer_id, index, view) in notepad.getFiles():
                notepad.activateFile(pathname)
                line_nbr = editor.markerNext(0, NPP_BOOKMARK_MARKER_MASK)
                bm_list = []
                while line_nbr != -1:
                    line_content = editor.getLine(line_nbr).strip()
                    bm_list.append((line_nbr, line_content))
                    line_nbr = editor.markerNext(line_nbr + 1, NPP_BOOKMARK_MARKER_MASK)
                if len(bm_list) > 0:
                    line_nbr_and_content_tup_by_path_dict[pathname] = bm_list
        if len(line_nbr_and_content_tup_by_path_dict) > 0:
            notepad.new()
            notepad.setLangType(LANGTYPE.PYTHON)  # provide fold points as a nicety
            eol = ['\r\n', '\r', '\n'][editor.getEOLMode()]
            editor.addText(self.header_line_to_look_for + eol)
            for pathname in line_nbr_and_content_tup_by_path_dict:
                directory = pathname.rsplit(os.sep, 1)[0]
                filename = pathname.rsplit(os.sep, 1)[-1]
                dir_to_show = '' if filename == directory else ' in "{D}{S}"'.format(D=directory, S=os.sep)
                editor.addText('  "{F}"{DTS}'.format(F=filename, DTS=dir_to_show) + eol)
                for bm in line_nbr_and_content_tup_by_path_dict[pathname]:
                    colon = ':' if len(bm[1]) > 0 else ''
                    editor.addText('\tLine {L}{c} {C}'.format(L=bm[0] + 1, c=colon, C=bm[1]) + eol)
            editor.setSavePoint()
            notepad.menuCommand(MENUCOMMAND.EDIT_SETREADONLY)

#-------------------------------------------------------------------------------

if __name__ == '__main__':
    try:
        biodl
    except NameError:
        biodl = BIODL()
    biodl.run()
