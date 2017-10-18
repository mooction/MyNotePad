#pragma once
#include "MarkdownParser.h"

const int MNP_PADDING = 20;							// padding 20px

LPCTSTR	  MNP_FONTFACE = L"Microsoft Yahei UI";		// L"Lucida Console";
const int MNP_FONTSIZE = 30;						// 25;
const int MNP_LINEHEIGHT = MNP_FONTSIZE;

const int MNP_BGCOLOR_EDIT = 0x00EEEEEE;
const int MNP_BGCOLOR_PREVIEW = 0x00F7F7F7;
const int MNP_BGCOLOR_SEL = 0x00CCCCCC;
const int MNP_FONTCOLOR = 0x00444444;

const int MNP_SCROLLBAR_BGCOLOR = 0x00E5E5E5;
const int MNP_SCROLLBAR_COLOR = 0x00D1D1D1;
const int MNP_SCROLLBAR_WIDTH = 10;

//---------------------------------

Article article;				// article text
Cursor caret(0, 0);			// current position = end of selection
Cursor sel_begin(0, 0);		// beginning of selection 

int textView_width = 0;
int textView_height = MNP_LINEHEIGHT;
int xoffset = 0;				// offset-x of textView
int yoffset = 0;				// offset-y of textView

//---------------------------------


// return true for success, false for no selected char.
bool removeSelectedChars() {
	if (sel_begin.n_line == caret.n_line)
	{
		if (sel_begin.n_character == caret.n_character)
			return false;
		else
			caret.eraseInLine(article, sel_begin.n_character);
	}
	else if (sel_begin.n_line < caret.n_line)	// forward selection
	{
		std::wstring str = sel_begin.subStringToEnd(article) +
			caret.subStringToStarting(article);
		article.erase(sel_begin.getSentence(article), caret.getSentence(article));
		caret.n_line = sel_begin.n_line;
		article.insert(caret.getSentence(article), Line(str));
	}
	else	// backward selection
	{
		std::wstring str = caret.subStringToEnd(article) +
			sel_begin.subStringToStarting(article);
		article.erase(caret.getSentence(article), sel_begin.getSentence(article));
		sel_begin.n_line = caret.n_line;
		article.insert(caret.getSentence(article), Line(str));
	}
	bSaved = false;
	return true;
}

// return the selected characters
std::wstring getSelectedChars() {
	if (sel_begin.n_line == caret.n_line)
	{
		if (sel_begin.n_character == caret.n_character)
			return L"";
		else
			return caret.subString(article, sel_begin.n_character);
	}
	else if (sel_begin.n_line < caret.n_line)	// forward selection
	{
		std::wstring str = sel_begin.subStringToEnd(article);
		for (auto& i = sel_begin.getSentence(article);
			i != caret.getSentence(article);)
			str.append(i->sentence.begin(), i->sentence.end());
		str += caret.subStringToStarting(article);
		return str;
	}
	else	// backward selection
	{
		std::wstring str = caret.subStringToEnd(article);
		for (auto& i = caret.getSentence(article);
			i != sel_begin.getSentence(article);)
			str.append(i->sentence.begin(), i->sentence.end());
		str += sel_begin.subStringToStarting(article);
		return str;
	}
}

//---------------------------------

// repaint edit area
void repaintView(HDC hdc) {
	caret_x = begin_x = 0;
	caret_y = begin_y = MNP_PADDING;

	int BUFFSIZE = article.size() + 1;
	TCHAR* text = new TCHAR[BUFFSIZE];

	{
		int char_width;
		Font f(MNP_FONTSIZE, MNP_FONTFACE);
		f.bind(hdc);	// for GetCharWidth32W()

		bool bCaretFound = false;
		bool bBeginFound = false;

		// p: TCHAR*	i: iterator
		TCHAR* p = text;
		auto i = article.begin();

		// generate full view
		for (; i != article.end() && p < text + BUFFSIZE - 1; ++p, ++i) {

			*p = *i;	// convert list<TCHAR> to TCHAR[]

			GetCharWidth32W(hdc, *p, *p, &char_width);

			if (i == caret) bCaretFound = true;
			if (!bCaretFound) {
				switch (*p) {				// calculate caret position
				case '\n':
					caret_y += MNP_LINEHEIGHT;				// line height
					caret_x = 0;							// new line
					break;
				default:
					caret_x += char_width;					// a-zA-z0-9
					break;
				}
			}
			if (sel_begin != caret) {
				if (i == sel_begin) bBeginFound = true;
				if (!bBeginFound) {
					switch (*p) {			// calculate sel_begin position
					case '\n':
						begin_y += MNP_LINEHEIGHT;			// line height
						begin_x = 0;						// new line
						break;
					default:
						begin_x += char_width;				// a-zA-z0-9
						break;
					}
				}
			}
		}
		*p = '\0';
		f.unbind();
	}

	// calculate width & height
	RECT rc;
	GetClientRect(hWnd, &rc);
	Font tf(MNP_FONTSIZE, MNP_FONTFACE);
	tf.bind(hdc).calcPrintArea(text, BUFFSIZE - 1, &textView_width, &textView_height).unbind();
	int canvasWidth = (rc.right - rc.left) / 2 + textView_width;
	int canvasHeight = rc.bottom - rc.top + textView_height;

	delete textView;
	textView = new MemDC(hdc, canvasWidth, canvasHeight);
	Font f(MNP_FONTSIZE, MNP_FONTFACE);						// set font
	f.setColor(MNP_FONTCOLOR).bind(*textView);

	// fill background
	GDIUtil::fill(*textView, MNP_BGCOLOR_EDIT, 0, 0, canvasWidth, canvasHeight);

	// fill selection background
	if (sel_begin != caret) {
		if (caret_y == begin_y && caret_x > begin_x)		// single line && forward
			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, begin_x, begin_y, caret_x - begin_x, MNP_LINEHEIGHT);
		else if (caret_y > begin_y)							// multi lines && forward
		{
			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, begin_x, begin_y, canvasWidth, MNP_LINEHEIGHT);

			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, 0, begin_y + MNP_LINEHEIGHT,
				canvasWidth, caret_y - begin_y - MNP_LINEHEIGHT);

			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, 0, caret_y, caret_x, MNP_LINEHEIGHT);
		}
		else if (caret_y == begin_y && caret_x < begin_x)	// single line && backward
			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, begin_x, begin_y, caret_x - begin_x, MNP_LINEHEIGHT);
		else if (caret_y < begin_y)							// multi lines && backward
		{
			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, caret_x, caret_y, canvasWidth, MNP_LINEHEIGHT);

			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, 0, caret_y + MNP_LINEHEIGHT,
				canvasWidth, begin_y - caret_y - MNP_LINEHEIGHT);

			GDIUtil::fill(*textView, MNP_BGCOLOR_SEL, 0, begin_y, begin_x, MNP_LINEHEIGHT);
		}
	}

	// print text
	f.print(text, BUFFSIZE - 1, 0, MNP_PADDING, textView_width, textView_height).unbind();

	delete[] text;

	// darw caret
	GDIUtil::line(*textView, MNP_FONTCOLOR, caret_x, caret_y, caret_x, caret_y + MNP_LINEHEIGHT);
}

void OnPaint(HDC hdc) {

	RECT rc;
	GetClientRect(hWnd, &rc);
	int ClientWidth = rc.right - rc.left;
	int ClientHeight = rc.bottom - rc.top;

	// create MemDC
	MemDC mdc = MemDC(hdc, rc.right - rc.left, rc.bottom - rc.top);

	// background color of edit area
	GDIUtil::fill(mdc, MNP_BGCOLOR_EDIT, 0, 0, ClientWidth, ClientHeight);

	// paste textView
	if (textView == nullptr) repaintView(mdc);
	BitBlt(mdc, MNP_PADDING, 0, ClientWidth, ClientHeight, *textView, xoffset, yoffset, SRCCOPY);

	// draw V scrollbar
	GDIUtil::fill(mdc, MNP_SCROLLBAR_BGCOLOR, ClientWidth - MNP_SCROLLBAR_WIDTH, 0,
		MNP_SCROLLBAR_WIDTH, ClientHeight);
	GDIUtil::fill(mdc, MNP_SCROLLBAR_COLOR, ClientWidth - MNP_SCROLLBAR_WIDTH, yoffset * ClientHeight / (textView_height + ClientHeight),
		MNP_SCROLLBAR_WIDTH, ClientHeight * ClientHeight / (textView_height + ClientHeight));
	// draw H scrollbar
	if (textView_width > ClientWidth - MNP_PADDING * 2) {
		GDIUtil::fill(mdc, MNP_SCROLLBAR_BGCOLOR, 0, ClientHeight - MNP_SCROLLBAR_WIDTH,
			ClientWidth, MNP_SCROLLBAR_WIDTH);
		GDIUtil::fill(mdc, MNP_SCROLLBAR_COLOR, xoffset * ClientWidth / (textView_width + ClientWidth), ClientHeight - MNP_SCROLLBAR_WIDTH,
			ClientWidth * ClientWidth / (textView_width + ClientWidth), MNP_SCROLLBAR_WIDTH);
	}

	// display
	BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, mdc, 0, 0, SRCCOPY);
}

// if caret out of sight, jump to caret
inline void seeCaret() {
	RECT rc;
	GetClientRect(hWnd, &rc);
	int ClientWidth = rc.right - rc.left;
	int ClientHeight = rc.bottom - rc.top;

	// x
	if (xoffset > caret_x)
		xoffset = 0;
	else if (xoffset < caret_x - ClientWidth + MNP_PADDING * 2)
		xoffset = caret_x - ClientWidth + MNP_PADDING * 2;

	// y
	if (yoffset > caret_y)
		yoffset = caret_y;
	else if (yoffset < caret_y - ClientHeight + MNP_LINEHEIGHT)
		yoffset = caret_y - ClientHeight + MNP_LINEHEIGHT;
}

void OnKeyDown(int nChar) {
	switch (nChar) {
	case VK_DELETE:
		if (removeSelectedChars())
			;
		else if (caret != article.end())
		{
			caret = article.erase(caret);
			bSaved = false;
		}
		else return;
		break;
	case VK_LEFT:
		if (caret != article.begin())
			--caret;		// backward
		else return;
		break;
	case VK_RIGHT:
		if (caret != article.end())
			++caret;		// forward
		else return;
		break;
	case VK_UP:
		if (caret != article.begin())
		{
			// upwards
			int n = 0;		// number of characters in front of the caret in this line
			auto i = caret;
			for (--i; i != article.begin(); --i) {
				if (*i == '\n')
				{
					if (*--i == '\n') {				// if previous line = "\n"
						caret = ++i;
						goto there;
					}
					toBeginOfLine(i);
					for (int t = 0; t < n && *i != '\n'; ++t) ++i;		// offset n
					caret = i;
					goto there;
				}
				++n;
			}
			if (*i == '\n') caret = i;
		}
		else return;
		break;
	case VK_DOWN:
		if (caret != article.end())
		{
			// downwards
			int n = 0;		// number of characters in front of the caret in this line
			auto i = caret;
			if (i != article.begin()) {					// count n
				for (--i; i != article.begin() && *i != '\n'; --i) ++n;
				if (i == article.begin() && *i != '\n') ++n;
				i = caret;
			}
			toEndOfLine(i);
			// now i = article.end() or '\n'
			if (i != article.end()) {
				for (int t = 0; t < n + 1 && i != --article.end() && *++i != '\n'; ++t)
					;		// goto next line, offset = n
				if (i == --article.end() && *i != '\n') ++i;
				caret = i;
			}
		}
		else return;
		break;
	case VK_HOME:
		if (caret == article.begin()) return;
		if (GetKeyState(VK_CONTROL) < 0)
			caret = article.begin();
		else
			caret.n_character = 0;
		break;
	case VK_END:
		if (caret == article.end())	return;
		if (GetKeyState(VK_CONTROL) < 0)
			caret = article.end();
		else
			toEndOfLine(caret);
		break;
	default:
		return;
	};
there:

	if (GetKeyState(VK_SHIFT) >= 0)
		sel_begin = caret;

	HDC hdc = GetDC(hWnd);
	repaintView(hdc);
	seeCaret();

	OnPaint(hdc);
	ReleaseDC(hWnd, hdc);
}

inline void OnChar(WORD nChar)
{
	// replace selected characters
	bool flag = removeSelectedChars();
	switch (nChar) {
	case VK_BACK:
		if (flag) break;
		if (caret != article.begin())
			caret = article.erase(--caret);
		break;
	case VK_RETURN:
		article.insert(caret, '\n');
		break;
	case VK_TAB:
	{
		Character c(L' ', );
		caret.insert(c);		// convert tab to space
		caret.insert(c);
		caret.insert(c);
		caret.insert(c);
	}
		break;
	default:
	{
		Character c(nChar, );
		caret.insert(c);
	}

	bSaved = false;
	sel_begin = caret;

	HDC hdc = GetDC(hWnd);
	repaintView(hdc);

	seeCaret();

	OnPaint(hdc);
	ReleaseDC(hWnd, hdc);
}

inline void OnKeyUp(int nChar) {
	;
}

auto getCaret(HDC hdc, int x, int y) {
	auto i = article.begin();
	{
		int cursor_y = y - MNP_PADDING + yoffset;
		if (cursor_y < 0)
			cursor_y = 0;
		else if (cursor_y > textView_height - 1)
			cursor_y = textView_height - MNP_LINEHEIGHT;

		// goto that line
		for (int nReturn = 0; nReturn < cursor_y / MNP_LINEHEIGHT; ++i)
			if (*i == '\n') ++nReturn;
	}

	Font f(MNP_FONTSIZE, MNP_FONTFACE);
	f.bind(hdc);	// for GetCharWidth32W()

	int char_width, total_width = 0;
	int cursor_x = x - MNP_PADDING + xoffset;
	if (cursor_x < 0) cursor_x = 0;
	while (i != article.end() && *i != '\n')
	{
		// goto that char
		GetCharWidth32W(hdc, *i, *i, &char_width);
		if (cursor_x > total_width + char_width / 2) {
			total_width += char_width;
			++i;
		}
		else break;
	}
	f.unbind();
	return i;
}

inline void OnLButtonDown(DWORD wParam, int x, int y) {
	HDC hdc = GetDC(hWnd);

	sel_begin = caret = getCaret(hdc, x, y);

	repaintView(hdc);
	OnPaint(hdc);
	ReleaseDC(hWnd, hdc);
}

inline void OnMouseMove(DWORD wParam, int x, int y) {
	if (wParam & MK_LBUTTON) {
		HDC hdc = GetDC(hWnd);
		std::list<TCHAR>::iterator new_caret = getCaret(hdc, x, y);
		if (caret != new_caret) {
			caret = new_caret;
			repaintView(hdc);
			OnPaint(hdc);
		}
		ReleaseDC(hWnd, hdc);
	}
}

inline void OnRButtonDown(DWORD wParam, int x, int y) {
	POINT p = { x,y };
	ClientToScreen(hWnd, &p);
	HMENU hm = LoadMenu(hInst, MAKEINTRESOURCE(IDC_POPUP));

	TrackPopupMenu(GetSubMenu(hm, 0), TPM_LEFTALIGN, p.x, p.y, NULL, hWnd, NULL);
	DestroyMenu(hm);
}

inline void OnMouseHWheel(short zDeta, int x, int y) {
	if (textView_width < MNP_FONTSIZE) return;
	xoffset += zDeta / 2;

	if (xoffset < 0)
		xoffset = 0;
	else if (xoffset > textView_width - MNP_FONTSIZE)
		xoffset = textView_width - MNP_FONTSIZE;

	HDC hdc = GetDC(hWnd);
	OnPaint(hdc);
	ReleaseDC(hWnd, hdc);
}

inline void OnMouseWheel(short zDeta, int x, int y) {
	if (GetKeyState(VK_SHIFT) < 0) {
		OnMouseHWheel(-zDeta, x, y);
		return;
	}
	yoffset -= zDeta / 2;

	if (yoffset < 0)
		yoffset = 0;
	else if (yoffset > textView_height)
		yoffset = textView_height;

	HDC hdc = GetDC(hWnd);
	OnPaint(hdc);
	ReleaseDC(hWnd, hdc);
}

#include <shellapi.h>
inline void OnMenuNew() {
	ShellExecuteW(hWnd, L"Open", L"MyNotePad.exe", NULL, NULL, SW_SHOWNORMAL);
}

#include <commdlg.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <locale>
#include <codecvt>
std::wstring opened_file = L"Untitled";	// full path

inline void OnMenuCopyHtml() {
	if (!OpenClipboard(hWnd)) return;
	if (!EmptyClipboard()) {
		CloseClipboard();
		return;
	}

	// !important
	Character c(L'\n', );
	article.front().sentence.push_front(c);
	article.back().sentence.push_back(c);

	// to HTML
	std::wstring str(article.begin(), article.end());
	parse_markdown(str);

	HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, str.size() * 2 + 2);
	LPTSTR p = static_cast<LPTSTR>(GlobalLock(h));
	for (TCHAR& c : str) {
		*p = c;
		++p;
	}
	*p = '\0';

	article.front().sentence.pop_front();
	article.back().sentence.pop_back();

	GlobalUnlock(h);
	SetClipboardData(CF_UNICODETEXT, h);
	CloseClipboard();
}

inline void setOFN(OPENFILENAME& ofn, LPCTSTR lpstrFilter, LPCTSTR lpstrDefExt) {
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = lpstrFilter;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = NULL;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = lpstrDefExt;
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
}

inline void OnMenuExport() {
	TCHAR file[MAX_PATH];		*file = '\0';

	OPENFILENAME ofn;
	ofn.lpstrFile = file;
	setOFN(ofn, L"HTML (*.html)\0*.html\0All Files (*.*)\0*.*\0\0", L"html");

	if (GetSaveFileNameW(&ofn) > 0) {

		// !important
		Character c(L'\n', );
		article.front().sentence.push_front(c);
		article.back().sentence.push_back(c);

		// write to file
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
		std::ofstream f(file);
		f << "<!DOCTYPE><head><meta charset=\"utf-8\"/><head><body>";
		std::wstring str(article.begin(), article.end());
		parse_markdown(str);
		f << cvt.to_bytes(str);
		f << "</body>";
		f.close();

		article.front().sentence.pop_front();
		article.back().sentence.pop_back();
	}
}

void OnMenuSaveAs() {
	TCHAR file[MAX_PATH];	*file = '\0';

	OPENFILENAME ofn;
	ofn.lpstrFile = file;
	setOFN(ofn, L"MarkDown (*.md)\0*.md\0All Files (*.*)\0*.*\0\0", L"md");

	if (GetSaveFileNameW(&ofn) > 0) {
		std::wstring str(article.begin(), article.end());

		std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
		std::ofstream f(file);
		f << cvt.to_bytes(str);
		f.close();

		opened_file = file;
		SetWindowTextW(hWnd, (opened_file + L" - MyNotePad").c_str());
		bSaved = true;
	}
}

inline void OnMenuSave() {
	if (opened_file == L"Untitled")
		OnMenuSaveAs();
	else
	{
		std::wstring str(article.begin(), article.end());

		std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
		std::ofstream f(opened_file);
		f << cvt.to_bytes(str);
		f.close();
		bSaved = true;
	}
}

// true: quit, false: cancel
bool sureToQuit() {
	if (!bSaved) {
		int r = MessageBoxW(hWnd, L"Save changes?", MNP_APPNAME, MB_YESNOCANCEL | MB_ICONQUESTION);
		if (r == IDCANCEL)
			return false;
		else if (r == IDYES) {
			OnMenuSave();
			return false;
		}
	}
	return true;
}

// repaintView() & OnPaint()
inline void refresh() {
	HDC hdc = GetDC(hWnd);
	repaintView(hdc);
	OnPaint(hdc);
	ReleaseDC(hWnd, hdc);
}

void loadFile(LPTSTR path) {
	// remove quote
	if (*path == '\"') {
		path[lstrlenW(path) - 1] = '\0';
		++path;
	}
	// load file
	std::ifstream f(path);
	if (f.fail()) {
		std::wstringstream ss;
		ss << L"Fail to load \"" << path << L"\".";
		MessageBoxW(hWnd, ss.str().c_str(), MNP_APPNAME, MB_OK | MB_ICONWARNING);
		return;
	}
	// UTF-8 decode
	std::istreambuf_iterator<char> begin(f), end;
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	std::wstring str;
	try {
		std::string bytes(begin, end);
		str = cvt.from_bytes(bytes);
	}
	catch (std::range_error re) {
		MessageBoxW(hWnd, L"Can only open UTF-8 file!", MNP_APPNAME, MB_OK | MB_ICONWARNING);
		return;
	}

	// clean
	article.clear();
	textView_width = 0;
	textView_height = MNP_LINEHEIGHT;
	caret = { 0,0 };
	for (auto i = str.begin(); i != str.end(); ++i) {
		if (*i == '\r')
		{
			;
		}
		else if (*i == '\t')
		{
			Character c(L' ', );
			caret.insert(c);		// convert tab to space
			caret.insert(c);
			caret.insert(c);
			caret.insert(c);
		}
		else
		{
			Character c(*i, );
			caret.insert(c);
		}
	}
	sel_begin = caret = { 0,0 };
	bSaved = true;

	opened_file = path;
	SetWindowTextW(hWnd, (opened_file + L" - MyNotePad").c_str());
	bSaved = true;

	refresh();
}

inline void OnMenuOpen() {
	sureToQuit();

	TCHAR file[MAX_PATH];		*file = '\0';
	OPENFILENAME ofn;
	ofn.lpstrFile = file;
	setOFN(ofn, L"MarkDown (*.md)\0*.md\0All Files (*.*)\0*.*\0\0", L"md");

	if (GetOpenFileNameW(&ofn) > 0) {
		loadFile(file);
	}
}

inline void OnMenuUndo() {

}

inline void OnMenuRedo() {

}

inline void OnMenuCopy() {
	if (sel_begin == caret) return;
	if (!OpenClipboard(hWnd)) return;
	if (!EmptyClipboard()) {
		CloseClipboard();
		return;
	}

	HGLOBAL h;
	std::wstring str = getSelectedChars();
	h = GlobalAlloc(GMEM_MOVEABLE, str.size() * 2 + 2);
	LPTSTR p = static_cast<LPTSTR>(GlobalLock(h));
	for (TCHAR& c : str)
		*p++ = c;
	*p = L'\0';

	GlobalUnlock(h);
	SetClipboardData(CF_UNICODETEXT, h);
	CloseClipboard();
}

inline void OnMenuCut() {
	OnMenuCopy();
	removeSelectedChars();
	refresh();
}

inline void OnMenuPaste() {

	if (!OpenClipboard(hWnd)) return;

	HANDLE h = GetClipboardData(CF_UNICODETEXT);
	if (NULL == h) {
		CloseClipboard();
		return;
	}

	removeSelectedChars();
	LPCTSTR str = (LPCTSTR)GlobalLock(h);

	while (*str != '\0')
	{
		if (*str == '\t')
		{
			Character c(L' ', );
			caret.insert(c);		// convert tab to space
			caret.insert(c);
			caret.insert(c);
			caret.insert(c);
		}
		else if (*str == '\r')
		{
			;
		}
		else
		{
			Character c(*str, );
			caret.insert(c);
		}
		++str;
	}

	GlobalUnlock(h);
	CloseClipboard();

	refresh();
}

inline void OnMenuAll() {
	sel_begin = { 0, 0 };
	caret.n_line = article.size() - 1;
	caret.n_character = article.end()->sentence.size();
	refresh();
}
