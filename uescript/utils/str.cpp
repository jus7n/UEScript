#include <uescript.h>
#include "str.h"

std::string StringUtl::WideToAsciiString(const std::wstring_view& string)
{
    const int size_required = WideCharToMultiByte(CP_ACP, 0, string.data(), -1, NULL, 0, NULL, NULL);
    std::string buffer(size_required, '\0');
    WideCharToMultiByte(CP_ACP, 0, string.data(), -1, buffer.data(), size_required, NULL, NULL);
    return buffer;
}

std::wstring StringUtl::AsciiToWideString(const std::string_view& string)
{
    const int size_required = MultiByteToWideChar(CP_ACP, 0, string.data(), -1, NULL, 0);
    std::wstring buffer(size_required, '\0');
    MultiByteToWideChar(CP_ACP, 0, string.data(), -1, buffer.data(), size_required);
    return buffer;
}

int StringUtl::WideToAsciiStringFast(const std::wstring_view& string, char* buffer, int buffer_size)
{
    const int size_required = WideCharToMultiByte(CP_ACP, 0, string.data(), -1, NULL, 0, NULL, NULL);
    UAssert(size_required + 1 < buffer_size);
    WideCharToMultiByte(CP_ACP, 0, string.data(), -1, buffer, size_required, NULL, NULL);
    return size_required;
}

int StringUtl::AsciiToWideStringFast(const std::string_view& string, wchar_t* buffer, int buffer_size)
{
    const int size_required = MultiByteToWideChar(CP_ACP, 0, string.data(), -1, NULL, 0);
    UAssert(size_required + 1 < buffer_size);
    MultiByteToWideChar(CP_ACP, 0, string.data(), -1, buffer, size_required);
    return size_required;
}
