#include "request.h"

#include <cwctype>

bool RuntimeRequest::GetString(
    const std::wstring& key,
    std::wstring& value
) const
{
    const std::wstring token =
        L"\"" + key + L"\"";

    size_t position =
        json.find(token);

    if (position == std::wstring::npos)
        return false;

    position =
        json.find(
            L':',
            position + token.length()
        );

    if (position == std::wstring::npos)
        return false;

    ++position;

    while (
        position < json.length() &&
        iswspace(json[position])
    )
    {
        ++position;
    }

    if (
        position >= json.length() ||
        json[position] != L'"'
    )
    {
        return false;
    }

    ++position;
    value.clear();

    while (position < json.length())
    {
        wchar_t ch =
            json[position++];

        if (ch == L'"')
            return true;

        if (
            ch == L'\\' &&
            position < json.length()
        )
        {
            wchar_t escaped =
                json[position++];

            switch (escaped)
            {
                case L'"':
                case L'\\':
                case L'/':
                    value.push_back(escaped);
                    break;

                case L'b':
                    value.push_back(L'\b');
                    break;

                case L'f':
                    value.push_back(L'\f');
                    break;

                case L'n':
                    value.push_back(L'\n');
                    break;

                case L'r':
                    value.push_back(L'\r');
                    break;

                case L't':
                    value.push_back(L'\t');
                    break;

                default:
                    value.push_back(escaped);
                    break;
            }

            continue;
        }

        value.push_back(ch);
    }

    return false;
}
