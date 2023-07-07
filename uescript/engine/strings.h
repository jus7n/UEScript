#pragma once
#include <cstdint>

struct FName
{
    union
    {
        struct
        {
            int32_t ComparisonIndex;
            int32_t Number;
        } i;

        uint64_t CompositeComparisonValue;
    } v;
};

struct FString
{
    const wchar_t* Data{nullptr};
    int32_t Count{0};
    int32_t Max{0};
};

/**
 * @brief Automatically manages FStrings returned from engine calls. 
 */
class UEStr final
{
public:
    explicit UEStr(const FString& engine_string)
        : m_Data{engine_string.Data}, m_Size{engine_string.Count}
    {
    }

    ~UEStr();

    // No copy constructors.

    UEStr& operator=(const UEStr&) = delete;
    UEStr(const UEStr&) = delete;

    std::wstring_view view() const
    {
        return {m_Data, static_cast<size_t>(m_Size)};
    }

    operator std::wstring_view() const
    {
        return view();
    }

private:
    const wchar_t* m_Data;
    int32_t m_Size;
};
