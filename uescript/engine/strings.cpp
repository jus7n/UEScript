#include <uescript.h>

#include "engine.h"
#include "strings.h"

UEStr::~UEStr()
{
    UAssert(m_Data);
    UE::FreeMemory(const_cast<wchar_t*>(m_Data));
}
