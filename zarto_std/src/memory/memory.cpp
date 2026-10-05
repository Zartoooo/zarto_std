#include "memory.h"

uintptr_t memory::rel_to_abs(uintptr_t address)
{
    return address + sizeof(int32_t) + *reinterpret_cast<int32_t*>(address);
}

std::uint8_t* memory::rel32(std::uint8_t* address)
{
    std::uint8_t* out;
    std::uint32_t r;

    out = address;

    r = *(std::uint32_t*)out;
    if (!r) return nullptr;

    out = (out + 4) + r;

    return out;
}

std::vector<signature*>& sig::get_sigs()
{
    static std::vector<signature*> sigs{};
    return sigs;
}

void sig::init()
{
    for (signature* entry : sig::get_sigs())
    {
        entry->setup();
    }
}
