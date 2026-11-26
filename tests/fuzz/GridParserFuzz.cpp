#include "BitGridCodec.h"

#include <cstdint>
#include <cstdlib>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
#ifdef BITGRID_FUZZ_RECOVER
    constexpr auto parse = BitGridCodec::recover;
    constexpr auto render = BitGridCodec::protect;
#else
    constexpr auto parse = BitGridCodec::decode;
    constexpr auto render = BitGridCodec::encode;
#endif
    const auto decoded = parse(std::string_view(reinterpret_cast<const char *>(data), size));
    if (const auto *payload = std::get_if<std::string>(&decoded))
    {
        // Every accepted payload must have a valid canonical rendering in the same format.
        const auto encoded = render(*payload);
        const auto *grid = std::get_if<std::string>(&encoded);
        if (!grid || parse(*grid) != decoded)
        {
            std::abort();
        }
    }
    return 0;
}
