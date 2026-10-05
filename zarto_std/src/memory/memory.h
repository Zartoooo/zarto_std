// internal memory for x64 windows

#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <Windows.h>

class signature;

namespace sig
{
    std::vector<signature*>& get_sigs();

    void init();
}

namespace memory
{
    template <typename T = uintptr_t, const std::uint16_t offset = 0u>
    inline T find_sig(const char* module_name, const char* signature)
    {
        if (const auto module_handle = GetModuleHandleA(module_name))
        {
            static auto signature_to_byte = [](const char* signature) {
                std::vector<int> bytes{ };
                auto start = const_cast<char*>(signature);
                auto end = const_cast<char*>(signature) + strlen(signature);

                for (auto current = start; current < end; ++current) {
                    if (*current == '?') {
                        ++current;

                        if (*current == '?')
                            ++current;

                        bytes.push_back(-1);
                    }

                    else bytes.push_back(static_cast<int>(strtoul(current, &current, 16)));
                }

                return bytes;
            };

            auto dos_header = reinterpret_cast<PIMAGE_DOS_HEADER>(module_handle);
            auto nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<std::uint8_t*>(module_handle) + dos_header->e_lfanew);

            const auto size_of_image = nt_headers->OptionalHeader.SizeOfImage;
            const auto pattern_bytes = signature_to_byte(signature);
            auto scan_bytes = reinterpret_cast<std::uint8_t*>(module_handle);

            const auto s = pattern_bytes.size();
            const auto d = pattern_bytes.data();

            for (std::size_t i = 0; i < size_of_image - s; ++i) {
                bool found = true;

                for (std::size_t j = 0; j < s; ++j) {
                    if (scan_bytes[i + j] != d[j] && d[j] != -1) {
                        found = false;
                        break;
                    }
                }

                if (found)
                    return reinterpret_cast<T>(&scan_bytes[i] + offset);
            }
        }

        return 0;
    }

    template <typename T = void*>
    inline T get_vfunc(void* ptr, const unsigned short index)
    {
        return (*static_cast<T**>(ptr))[index];
    }

    template <typename Ret, typename ...Args>
    Ret call_vfunc(void* self, int idx, Args... args)
    {
        using fn = Ret(__fastcall*)(void*, decltype(args)...);
        return reinterpret_cast<fn>((*static_cast<void***>(self))[idx])(self, args...);
    }

    inline uintptr_t find_export(std::string dll, std::string fn)
    {
        const HMODULE hmod{ GetModuleHandleA(dll.c_str()) };

        if (!hmod) {
            return 0;
        }

        return reinterpret_cast<uintptr_t>(GetProcAddress(hmod, fn.c_str()));
    }

    uintptr_t rel_to_abs(uintptr_t address);

    std::uint8_t* rel32(std::uint8_t* address);
}

class signature
{
public:
    signature(const char* module, const char* name, const char* pattern)
        : m_name(name), m_pattern(pattern), m_module(module), m_address(0u)
    {
        sig::get_sigs().push_back(this);
    }

    bool is_found() const { return m_address != 0u; }

    void setup()
    {
        m_address = memory::find_sig(m_module.c_str(), m_pattern.c_str());
    }

    signature& offset(std::ptrdiff_t offset)
    {
        m_address += offset;
        return *this;
    }

    signature& rel_to_abs()
    {
        m_address = memory::rel_to_abs(m_address);
        return *this;
    }

    signature& rel_to_abs(std::ptrdiff_t opcode_offset)
    {
        m_address += opcode_offset;
        m_address = memory::rel_to_abs(m_address);
        return *this;
    }

    signature& rel32()
    {
        m_address = reinterpret_cast<std::uintptr_t>(memory::rel32(reinterpret_cast<std::uint8_t*>(m_address)));
        return *this;
    }

    signature& dereference(std::size_t count = 1u)
    {
        while (count--)
            m_address = *reinterpret_cast<std::uintptr_t*>(m_address);
        return *this;
    }

    template <typename T>
    T* get_ptr() const
    {
        return reinterpret_cast<T*>(m_address);
    }

    template <typename T = uintptr_t>
    T get() const
    {
        return *reinterpret_cast<T*>(m_address);
    }

    std::uintptr_t get_raw() const
    {
        return m_address;
    }

    std::string m_name;
    std::string m_pattern;
    std::string m_module;
    std::uintptr_t m_address;
};

#define MAKE_SIGNATURE(symbol, dll, pattern) \
namespace sig \
{ \
    inline signature symbol = signature(dll, #symbol, pattern); \
}
