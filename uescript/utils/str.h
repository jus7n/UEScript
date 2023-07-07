#pragma once
#include <string>
#include <array>

/**
 * @brief String utilities
 */
class StringUtl final
{
public:
    StringUtl() = delete;
    ~StringUtl() = delete;

    /**
     * @brief Convert a wide string to an ascii string. Result is allocated.
     */
    static std::string WideToAsciiString(const std::wstring_view& string);

    /**
     * @brief Convert an ascii string to a wide string. Result is allocated.
     */
    static std::wstring AsciiToWideString(const std::string_view& string);

    /**
     * @brief Convert a wide string to an ascii string. Result is converted in-place in the specified buffer.
     */
    static int WideToAsciiStringFast(const std::wstring_view& string, char* buffer, int buffer_size);

    /**
     * @brief Convert an ascii string to a wide string. Result is converted in-place in the specified buffer.
     */
    static int AsciiToWideStringFast(const std::string_view& string, wchar_t* buffer, int buffer_size);

    template <int Size>
    constexpr static std::string_view WideToAsciiStringFast(const std::wstring_view& string,
                                                            std::array<char, Size>& buffer)
    {
        const int size = WideToAsciiStringFast(string, buffer.data(), Size);
        return std::string_view(buffer.data(), size);
    }

    template <int Size>
    constexpr static std::wstring_view AsciiToWideStringFast(const std::string_view& string,
                                                             std::array<wchar_t, Size>& buffer)
    {
        const int size = AsciiToWideStringFast(string, buffer.data(), Size);
        return std::wstring_view(buffer.data(), size);
    }
};

/**
 * @brief Thread-local buffers to be used when a only a single string conversion result is required at a time and is
 *		  immediately copied somewhere else (pushing onto Lua stack).
 */
namespace conv_buf
{
    static thread_local std::array<wchar_t, 512> g_WideBuf;
    static thread_local std::array<char, 512> g_AsciiBuf;
}
