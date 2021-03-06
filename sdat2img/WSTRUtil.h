﻿#pragma once
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
namespace WSTRUtils
{
	static std::string wchar_to_UTF8(std::wstring In)
	{
		const wchar_t* in = In.c_str();
		std::string out;
		unsigned int codepoint = 0;
		for (in; *in != 0; ++in)
		{
			if (*in >= 0xd800 && *in <= 0xdbff)
				codepoint = ((*in - 0xd800) << 10) + 0x10000;
			else
			{
				if (*in >= 0xdc00 && *in <= 0xdfff)
					codepoint |= *in - 0xdc00;
				else
					codepoint = *in;

				if (codepoint <= 0x7f)
					out.append(1, static_cast<char>(codepoint));
				else if (codepoint <= 0x7ff)
				{
					out.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
					out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
				}
				else if (codepoint <= 0xffff)
				{
					out.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
					out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
					out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
				}
				else
				{
					out.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
					out.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
					out.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
					out.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
				}
				codepoint = 0;
			}
		}
		return out;
	}

	static std::wstring UTF8_to_wchar(std::string In)
	{
		const char* in = In.c_str();
		std::wstring out;
		unsigned int codepoint = 0;
		while (*in != 0)
		{
			unsigned char ch = static_cast<unsigned char>(*in);
			if (ch <= 0x7f)
				codepoint = ch;
			else if (ch <= 0xbf)
				codepoint = (codepoint << 6) | (ch & 0x3f);
			else if (ch <= 0xdf)
				codepoint = ch & 0x1f;
			else if (ch <= 0xef)
				codepoint = ch & 0x0f;
			else
				codepoint = ch & 0x07;
			++in;
			if (((*in & 0xc0) != 0x80) && (codepoint <= 0x10ffff))
			{
				if (sizeof(wchar_t) > 2)
					out.append(1, static_cast<wchar_t>(codepoint));
				else if (codepoint > 0xffff)
				{
					out.append(1, static_cast<wchar_t>(0xd800 + (codepoint >> 10)));
					out.append(1, static_cast<wchar_t>(0xdc00 + (codepoint & 0x03ff)));
				}
				else if (codepoint < 0xd800 || codepoint >= 0xe000)
					out.append(1, static_cast<wchar_t>(codepoint));
			}
		}
		return out;
	}

	static inline char from_hex(char ch) {
		return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
	}
	static std::string url_decode(std::string text) {
		std::ostringstream escaped;
		escaped.fill('0');
		char h = 0;
		for (auto i = text.begin(), n = text.end(); i != n; ++i) {
			std::string::value_type c = (*i);

			if (c == '%') {
				if (i[1] && i[2]) {
					h = from_hex(i[1]) << 4 | from_hex(i[2]);
					escaped << h;
					i += 2;
				}
			}
			else if (c == '+')
				escaped << ' ';
			else
				escaped << c;
		}
		return escaped.str();
	}

	static std::string url_encode(std::string value) {
		std::ostringstream escaped;
		escaped.fill('0');
		escaped << std::hex;
		for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
			std::string::value_type cChar = (*i);
			if (((cChar >= '0' && cChar <= '9') || (cChar >= 'A' && cChar <= 'Z') || (cChar >= 'a' && cChar <= 'z')) || cChar == '-' || cChar == '_' || cChar == '.' || cChar == '~') {
				escaped << cChar;
				continue;
			}
			if (cChar == ' ')
			{
				escaped << '+';
				continue;
			}
			escaped << std::uppercase;
			escaped << '%' << std::setw(2) << int((unsigned char)cChar);
			escaped << std::nouppercase;
		}
		return escaped.str();
	}

	static std::wstring ReplaceStringAll(std::wstring str, const std::wstring& from, const std::wstring& to) {
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
		return str;
	}

	static std::wstring ReplaceSymbsAll(std::wstring str, const std::wstring& from, const wchar_t to) {
		for (auto &s : from)
		{
			size_t start_pos = 0;
			while ((start_pos = str.find(s, start_pos)) != std::wstring::npos) {
				str.replace(start_pos, 1, 1, to);
				start_pos++;
			}
		}
		return str;
	}

	static std::wstring FixFileName(std::wstring Path)
	{
		for (auto& i : Path)
		{
			if(i >= 0x0001U && i < 0x001FU)
				i = L' ';
			else
			{
				switch (i)
				{
				case L'\\':
				case L'/':
				case L':':
				case L'*':
				case L'?':
				case L'\"':
				case L'<':
				case L'>':
				case L'|':
					i = L' ';
					break;
				}
			}
		}

		return Path;
	}

	static std::string FixFileName(std::string Path)
	{
		for (auto& i : Path)
		{
			if (i >= 0x01U && i < 0x1FU)
				i = ' ';
			else
			{
				switch (i)
				{
				case '\\':
				case '/':
				case ':':
				case '*':
				case '?':
				case '\"':
				case '<':
				case '>':
				case '|':
					i = ' ';
					break;
				}
			}
		}

		return Path;
	}

	static std::wstring FixFileNameNotSlash(std::wstring Path)
	{
		for (auto& i : Path)
		{
			if (i >= 0x0001U && i < 0x001FU)
				i = L' ';
			else
			{
				switch (i)
				{
				case L':':
				case L'*':
				case L'?':
				case L'\"':
				case L'<':
				case L'>':
				case L'|':
					i = L' ';
					break;
				}
			}
		}

		return Path;
	}

	static std::string FixFileNameNotSlash(std::string Path)
	{
		for (auto& i : Path)
		{
			if (i >= 0x01U && i < 0x1FU)
				i = ' ';
			else
			{
				switch (i)
				{
				case ':':
				case '*':
				case '?':
				case '\"':
				case '<':
				case '>':
				case '|':
					i = ' ';
					break;
				}
			}
		}

		return Path;
	}

	static wchar_t ToWLower(wchar_t c)
	{
		if (c >= L'А' && c <= L'Я')
			return c + (L'я' > L'Я' ? L'я' - L'Я' : L'Я' - L'я');
		if (c >= L'A' && c <= L'Z')
			return c + (L'z' > L'Z' ? L'z' - L'Z' : L'Z' - L'z');
		return c;
	}

	static char ToLower(char c)
	{
		if (c >= 'A' && c <= 'Z')
			return c + ('z' > 'Z' ? 'z' - 'Z' : 'Z' - 'z');
		return c;
	}

	static bool compare(std::string Pattern, std::string Cmp)
	{
		if (Pattern.length() < Cmp.length())
			return false;
		for (size_t i = 0; i < Cmp.length(); i++)
		{
			if (ToLower(Pattern[i]) != ToLower(Cmp[i]))
				return false;
		}
		return true;
	}

	static bool Wcompare(std::wstring Pattern, std::wstring Cmp)
	{
		if (Pattern.length() < Cmp.length())
			return false;
		for (size_t i = 0; i < Cmp.length(); i++)
		{
			if (ToWLower(Pattern[i]) != ToWLower(Cmp[i]))
				return false;
		}
		return true;
	}

	static size_t wsearch(std::wstring &Buf, std::wstring Find)
	{
		int dwOffset = 0;
		while (dwOffset + (int)Find.length() <= (int)Buf.length())
		{
			if (Wcompare(&Buf[dwOffset], Find) == true)
				return dwOffset;
			dwOffset++;
		}
		return std::string::npos;
	}

	static size_t search(std::string &Buf, std::string Find)
	{
		int dwOffset = 0;
		while (dwOffset + (int)Find.length() <= (int)Buf.length())
		{
			if (compare(&Buf[dwOffset], Find) == true)
				return dwOffset;
			dwOffset++;
		}
		return std::string::npos;
	}

	static size_t not_const_wsearch(std::wstring Buf, std::wstring Find)
	{
		int dwOffset = 0;
		while (dwOffset + (int)Find.length() <= (int)Buf.length())
		{
			if (Wcompare(&Buf[dwOffset], Find) == true)
				return dwOffset;
			dwOffset++;
		}
		return std::string::npos;
	}

	static size_t not_const_search(std::string Buf, std::string Find)
	{
		int dwOffset = 0;
		while (dwOffset + (int)Find.length() <= (int)Buf.length())
		{
			if (compare(&Buf[dwOffset], Find) == true)
				return dwOffset;
			dwOffset++;
		}
		return std::string::npos;
	}

	static std::string GetStringDate(tm* t_m)
	{
		char Week[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		char Month[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		char Strbuf[80];

		memset(Strbuf, 0, sizeof(Strbuf));
		sprintf_s(Strbuf, "%s, %d %s %d %d:%d:%d GMT", Week[(*t_m).tm_wday], (*t_m).tm_mday, Month[(*t_m).tm_mon], 1900 + (*t_m).tm_year, (*t_m).tm_hour, (*t_m).tm_min, (*t_m).tm_sec);
		return Strbuf;
	}

	static std::wstring WGetStringDate(tm* t_m)
	{
		wchar_t Week[7][4] = { L"Вс", L"Пн", L"Вт", L"Ср", L"Чт", L"Пт", L"Сб" };
		wchar_t Month[12][4] = { L"Янв", L"Фев", L"Мар", L"Апр", L"Май", L"Июн", L"Июл", L"Авг", L"Сен", L"Окт", L"Ноя", L"Дек" };
		wchar_t Strbuf[80];

		memset(Strbuf, 0, sizeof(Strbuf));
		swprintf_s(Strbuf, L"%s. %d %s %d %d.%d.%d", Week[(*t_m).tm_wday], (*t_m).tm_mday, Month[(*t_m).tm_mon], 1900 + (*t_m).tm_year, (*t_m).tm_hour, (*t_m).tm_min, (*t_m).tm_sec);
		return Strbuf;
	}

	static std::string GetTimeGMT(int SkipDay)
	{
		tm u;
		const time_t timer = time(NULL);
		gmtime_s(&u, &timer);
		u.tm_mday += SkipDay;
		mktime(&u);
		return GetStringDate(&u);
	}

	static std::wstring GetTimeLocal()
	{
		tm u;
		const time_t timer = time(NULL);
		localtime_s(&u, &timer);
		return WGetStringDate(&u);
	}

	static std::wstring GetTimeAT(time_t timer)
	{
		tm u;
		localtime_s(&u, &timer);
		return WGetStringDate(&u);
	}

	static std::string urlencodewchar(std::wstring link)
	{
		return url_encode(wchar_to_UTF8(link));
	}
	static std::string wchar_to_Cp1251(const std::wstring In)
	{
		std::string ret;
		ret.reserve(In.size());
		std::wstring::const_iterator it = In.begin(), it_e = In.end();
		for (; it != it_e; ++it) {
			wchar_t s = *it;
			char c = 0;
			if (s == 0x0451)
				c = char(184);
			else if (s == 0x0401)
				c = char(168);
			else if (s >= 0x350 + 0xc0 && s <= 0x350 + 0xff)
				c = char(s - 0x350);
			else if (s < 0xc0)
				c = char(s);
			else
				c = ' ';
			if (c)
				ret.push_back(c);
		}
		return ret;
	}

	static std::string UTF8_to_Cp1251(const std::string& In)
	{
		return wchar_to_Cp1251(UTF8_to_wchar(In));
	}
}