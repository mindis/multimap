// This file is part of the MT library.
//
// Copyright (C) 2015-2016  Martin Trenkmann
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MT_UNICODE_H_
#define MT_UNICODE_H_

#include <string>

namespace mt {

void setUtf8Locale();

void toUtf8(const std::wstring& utf32, std::string* utf8);

std::string toUtf8Copy(const std::wstring& utf32);

void toUtf32(const std::string& utf8, std::wstring* utf32);

std::wstring toUtf32Copy(const std::string& utf8);

void toLowerUtf8(std::string* utf8);

std::string toLowerUtf8Copy(const std::string& utf8);

void toLowerUtf32(std::wstring* utf32);

std::wstring toLowerUtf32Copy(const std::wstring& utf32);

void toUpperUtf8(std::string* utf8);

std::string toUpperUtf8Copy(const std::string& utf8);

void toUpperUtf32(std::wstring* utf32);

std::wstring toUpperUtf32Copy(const std::wstring& utf32);

}  // namespace mt

#endif  // MT_UNICODE_H_
