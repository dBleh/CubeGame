#ifndef STEAM_HELPERS_H
#define STEAM_HELPERS_H

#include <steam/steam_api.h>
#include <cstdint>
#include <functional>
#include <unordered_map>

/**
 * @brief Hash functor for CSteamID.
 *
 * Converts a CSteamID to a uint64_t and returns its hash value.
 */
struct CSteamIDHash {
    std::size_t operator()(const CSteamID& id) const {
        return std::hash<uint64_t>{}(id.ConvertToUint64());
    }
};

#endif // STEAM_HELPERS_H
