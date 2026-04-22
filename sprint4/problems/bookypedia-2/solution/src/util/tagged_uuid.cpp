#include "tagged_uuid.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace util {
namespace detail {

UUIDType NewUUID() {
    return boost::uuids::random_generator()();
}

std::string UUIDToString(const UUIDType& uuid) {
    return to_string(uuid);
}

UUIDType UUIDFromString(std::string_view str) {
    boost::uuids::string_generator gen;
    return gen(str.begin(), str.end());
}

}  // namespace detail

std::string NormalizeSpaces(std::string s) {
    std::stringstream ss(s);
    std::string word, result;
    while (ss >> word) {
        if (!result.empty()) result += " ";
        result += word;
    }
    return result;
}

std::vector<std::string> NormalizeTags(const std::string& input) {
    std::stringstream ss(input);
    std::string tag;
    std::set<std::string> unique;

    while (std::getline(ss, tag, ',')) {
        boost::algorithm::trim(tag);
        tag = NormalizeSpaces(tag);

        if (!tag.empty()) {
            unique.insert(tag);
        }
    }

    return {unique.begin(), unique.end()};
}
}  // namespace util
