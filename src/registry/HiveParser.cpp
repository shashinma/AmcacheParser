#include "HiveParser.h"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace amcache {

// ============================================================================
// HiveValue
// ============================================================================

std::optional<std::string> HiveValue::GetString() const {
    if (!valid_ || data_.empty()) {
        return std::nullopt;
    }

    if (type_ == RegValueType::REG_SZ ||
        type_ == RegValueType::REG_EXPAND_SZ ||
        type_ == RegValueType::REG_LINK) {
        if (data_.size() >= 2) {
            std::wstring wstr;
            for (size_t i = 0; i + 1 < data_.size(); i += 2) {
                wchar_t ch = static_cast<wchar_t>(data_[i] | (data_[i + 1] << 8));
                if (ch == 0) break;
                wstr += ch;
            }

            std::string result;
            result.reserve(wstr.size() * 3);
            for (wchar_t wc : wstr) {
                if (wc < 0x80) {
                    result += static_cast<char>(wc);
                } else if (wc < 0x800) {
                    result += static_cast<char>(0xC0 | (wc >> 6));
                    result += static_cast<char>(0x80 | (wc & 0x3F));
                } else {
                    result += static_cast<char>(0xE0 | (wc >> 12));
                    result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (wc & 0x3F));
                }
            }
            return result;
        }
    }

    std::string result;
    for (uint8_t b : data_) {
        if (b == 0) break;
        result += static_cast<char>(b);
    }
    return result;
}

std::optional<int32_t> HiveValue::GetInt32() const {
    if (!valid_) return std::nullopt;
    if (type_ == RegValueType::REG_DWORD && data_.size() >= 4) {
        int32_t value = 0;
        std::memcpy(&value, data_.data(), 4);
        return value;
    }
    return std::nullopt;
}

std::optional<uint32_t> HiveValue::GetUInt32() const {
    if (!valid_) return std::nullopt;
    if (type_ == RegValueType::REG_DWORD && data_.size() >= 4) {
        uint32_t value = 0;
        std::memcpy(&value, data_.data(), 4);
        return value;
    }
    return std::nullopt;
}

std::optional<int64_t> HiveValue::GetInt64() const {
    if (!valid_) return std::nullopt;
    if (type_ == RegValueType::REG_QWORD && data_.size() >= 8) {
        int64_t value = 0;
        std::memcpy(&value, data_.data(), 8);
        return value;
    }
    if (type_ == RegValueType::REG_DWORD && data_.size() >= 4) {
        int32_t value = 0;
        std::memcpy(&value, data_.data(), 4);
        return static_cast<int64_t>(value);
    }
    return std::nullopt;
}

std::optional<uint64_t> HiveValue::GetUInt64() const {
    if (!valid_) return std::nullopt;
    if (type_ == RegValueType::REG_QWORD && data_.size() >= 8) {
        uint64_t value = 0;
        std::memcpy(&value, data_.data(), 8);
        return value;
    }
    if (type_ == RegValueType::REG_DWORD && data_.size() >= 4) {
        uint32_t value = 0;
        std::memcpy(&value, data_.data(), 4);
        return static_cast<uint64_t>(value);
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> HiveValue::GetBinary() const {
    if (!valid_) return std::nullopt;
    return data_;
}

// ============================================================================
// HiveKey
// ============================================================================

static bool CaseInsensitiveEqual(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

HiveKey* HiveKey::GetSubKeyByName(const std::string& name) {
    for (auto& subkey : subKeys_) {
        if (CaseInsensitiveEqual(subkey.name_, name)) {
            return &subkey;
        }
    }
    return nullptr;
}

HiveValue* HiveKey::GetValueByName(const std::string& name) {
    for (auto& value : values_) {
        if (CaseInsensitiveEqual(value.GetName(), name)) {
            return &value;
        }
    }
    return nullptr;
}

std::optional<std::string> HiveKey::GetStringValue(const std::string& name) const {
    for (const auto& value : values_) {
        if (CaseInsensitiveEqual(value.GetName(), name)) {
            return value.GetString();
        }
    }
    return std::nullopt;
}

std::optional<int32_t> HiveKey::GetInt32Value(const std::string& name) const {
    for (const auto& value : values_) {
        if (CaseInsensitiveEqual(value.GetName(), name)) {
            return value.GetInt32();
        }
    }
    return std::nullopt;
}

std::optional<uint32_t> HiveKey::GetUInt32Value(const std::string& name) const {
    for (const auto& value : values_) {
        if (CaseInsensitiveEqual(value.GetName(), name)) {
            return value.GetUInt32();
        }
    }
    return std::nullopt;
}

std::optional<int64_t> HiveKey::GetInt64Value(const std::string& name) const {
    for (const auto& value : values_) {
        if (CaseInsensitiveEqual(value.GetName(), name)) {
            return value.GetInt64();
        }
    }
    return std::nullopt;
}

std::optional<uint64_t> HiveKey::GetUInt64Value(const std::string& name) const {
    for (const auto& value : values_) {
        if (CaseInsensitiveEqual(value.GetName(), name)) {
            return value.GetUInt64();
        }
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> HiveKey::GetBinaryValue(const std::string& name) const {
    for (const auto& value : values_) {
        if (CaseInsensitiveEqual(value.GetName(), name)) {
            return value.GetBinary();
        }
    }
    return std::nullopt;
}

// ============================================================================
// HiveParser
// ============================================================================

bool HiveParser::Open(const std::string& path) {
    Close();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    data_.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(data_.data()), size)) {
        return false;
    }

    return ParseHive();
}

bool HiveParser::OpenFromBuffer(const std::vector<uint8_t>& buffer) {
    Close();
    data_ = buffer;
    return ParseHive();
}

void HiveParser::Close() {
    data_.clear();
    rootKey_ = HiveKey();
    isOpen_ = false;
    isDirty_ = false;
    primarySequence_ = 0;
    secondarySequence_ = 0;
}

bool HiveParser::ParseHive() {
    if (data_.size() < sizeof(HiveBaseBlock)) {
        return false;
    }

    HiveBaseBlock baseBlock;
    std::memcpy(&baseBlock, data_.data(), sizeof(HiveBaseBlock));

    if (baseBlock.signature != 0x66676572) {
        return false;
    }

    primarySequence_ = baseBlock.primarySequence;
    secondarySequence_ = baseBlock.secondarySequence;
    isDirty_ = (baseBlock.primarySequence != baseBlock.secondarySequence);

    if (!ParseKeyNode(baseBlock.rootCellOffset, rootKey_)) {
        return false;
    }

    isOpen_ = true;
    return true;
}

bool HiveParser::ParseKeyNode(uint32_t offset, HiveKey& key) {
    int32_t cellSize;
    if (!ReadAt(offset, cellSize)) {
        return false;
    }

    KeyNode node;
    if (!ReadBytes(offset + 4, &node, sizeof(KeyNode))) {
        return false;
    }

    if (node.signature != 0x6B6E) {
        return false;
    }

    // Skip deleted keys unless recover deleted is enabled
    if ((node.flags & 0x01) != 0 && !recoverDeleted_) {
        return false;
    }

    key.name_ = ReadString(offset + 4 + sizeof(KeyNode), node.keyNameLen, (node.flags & 0x20) != 0);
    key.lastWriteTime_ = FileTimeToTimePoint(node.lastWriteTime);
    key.valid_ = true;

    if (node.subKeyCount > 0 && node.subKeyListOffset != 0xFFFFFFFF) {
        ParseSubKeys(node.subKeyListOffset, node.subKeyCount, key);
    }

    if (node.valueCount > 0 && node.valueListOffset != 0xFFFFFFFF) {
        ParseValues(node.valueListOffset, node.valueCount, key);
    }

    return true;
}

bool HiveParser::ParseSubKeys(uint32_t listOffset, uint32_t count, HiveKey& parentKey) {
    int32_t cellSize;
    if (!ReadAt(listOffset, cellSize)) {
        return false;
    }

    uint16_t sig;
    if (!ReadAt(listOffset + 4, sig)) {
        return false;
    }

    parentKey.subKeys_.reserve(count);

    if (sig == 0x666C || sig == 0x686C) {
        uint16_t numElements;
        if (!ReadAt(listOffset + 6, numElements)) {
            return false;
        }

        for (uint16_t i = 0; i < numElements && i < count; ++i) {
            uint32_t keyOffset;
            if (!ReadAt(listOffset + 8 + i * 8, keyOffset)) {
                continue;
            }

            HiveKey subKey;
            if (ParseKeyNode(keyOffset, subKey)) {
                parentKey.subKeys_.push_back(std::move(subKey));
            }
        }
    } else if (sig == 0x696C) {
        uint16_t numElements;
        if (!ReadAt(listOffset + 6, numElements)) {
            return false;
        }

        for (uint16_t i = 0; i < numElements && i < count; ++i) {
            uint32_t keyOffset;
            if (!ReadAt(listOffset + 8 + i * 4, keyOffset)) {
                continue;
            }

            HiveKey subKey;
            if (ParseKeyNode(keyOffset, subKey)) {
                parentKey.subKeys_.push_back(std::move(subKey));
            }
        }
    } else if (sig == 0x6972) {
        uint16_t numElements;
        if (!ReadAt(listOffset + 6, numElements)) {
            return false;
        }

        for (uint16_t i = 0; i < numElements; ++i) {
            uint32_t subListOffset;
            if (!ReadAt(listOffset + 8 + i * 4, subListOffset)) {
                continue;
            }
            ParseSubKeys(subListOffset, count, parentKey);
        }
    }

    return true;
}

bool HiveParser::ParseValues(uint32_t listOffset, uint32_t count, HiveKey& key) {
    key.values_.reserve(count);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t valueOffset;
        if (!ReadAt(listOffset + 4 + i * 4, valueOffset)) {
            continue;
        }

        HiveValue value;
        if (ParseValue(valueOffset, value)) {
            key.values_.push_back(std::move(value));
        }
    }

    return true;
}

bool HiveParser::ParseValue(uint32_t offset, HiveValue& value) {
    int32_t cellSize;
    if (!ReadAt(offset, cellSize)) {
        return false;
    }

    ValueNode node;
    if (!ReadBytes(offset + 4, &node, sizeof(ValueNode))) {
        return false;
    }

    if (node.signature != 0x6B76) {
        return false;
    }

    if (node.nameLen > 0) {
        value.name_ = ReadString(offset + 4 + sizeof(ValueNode), node.nameLen, false);
    } else {
        value.name_ = "(Default)";
    }

    value.type_ = static_cast<RegValueType>(node.dataType);

    uint32_t dataSize = node.dataSize & 0x7FFFFFFF;
    bool isResident = (node.dataSize & 0x80000000) != 0;

    if (dataSize > 0) {
        if (isResident && dataSize <= 4) {
            value.data_.resize(dataSize);
            std::memcpy(value.data_.data(), &node.dataOffset, dataSize);
        } else if (node.dataOffset != 0xFFFFFFFF) {
            value.data_.resize(dataSize);
            ReadBytes(node.dataOffset + 4, value.data_.data(), dataSize);
        }
    }

    value.valid_ = true;
    return true;
}

template<typename T>
bool HiveParser::ReadAt(uint32_t offset, T& value) {
    size_t pos = hiveBinsOffset_ + offset;
    if (pos + sizeof(T) > data_.size()) {
        return false;
    }
    std::memcpy(&value, data_.data() + pos, sizeof(T));
    return true;
}

bool HiveParser::ReadBytes(uint32_t offset, void* buffer, size_t size) {
    size_t pos = hiveBinsOffset_ + offset;
    if (pos + size > data_.size()) {
        return false;
    }
    std::memcpy(buffer, data_.data() + pos, size);
    return true;
}

std::string HiveParser::ReadString(uint32_t offset, uint32_t length, bool isUtf16) {
    size_t pos = hiveBinsOffset_ + offset;
    if (pos + length > data_.size()) {
        return "";
    }

    if (isUtf16) {
        std::wstring wstr;
        for (uint32_t i = 0; i + 1 < length; i += 2) {
            wchar_t ch = static_cast<wchar_t>(data_[pos + i] | (data_[pos + i + 1] << 8));
            if (ch == 0) break;
            wstr += ch;
        }

        std::string result;
        result.reserve(wstr.size() * 3);
        for (wchar_t wc : wstr) {
            if (wc < 0x80) {
                result += static_cast<char>(wc);
            } else if (wc < 0x800) {
                result += static_cast<char>(0xC0 | (wc >> 6));
                result += static_cast<char>(0x80 | (wc & 0x3F));
            } else {
                result += static_cast<char>(0xE0 | (wc >> 12));
                result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (wc & 0x3F));
            }
        }
        return result;
    }

    return std::string(reinterpret_cast<const char*>(data_.data() + pos), length);
}

std::chrono::system_clock::time_point HiveParser::FileTimeToTimePoint(uint64_t filetime) {
    constexpr uint64_t FILETIME_UNIX_DIFF = 116444736000000000ULL;

    if (filetime < FILETIME_UNIX_DIFF) {
        return std::chrono::system_clock::time_point{};
    }

    uint64_t unix_100ns = filetime - FILETIME_UNIX_DIFF;
    auto duration = std::chrono::duration<uint64_t, std::ratio<1, 10000000>>(unix_100ns);
    return std::chrono::system_clock::time_point(
        std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
}

HiveKey* HiveParser::GetKeyByPath(const std::string& path) {
    if (!isOpen_) return nullptr;

    HiveKey* current = &rootKey_;
    size_t start = 0;
    size_t end = 0;

    while ((end = path.find('\\', start)) != std::string::npos) {
        std::string part = path.substr(start, end - start);
        if (!part.empty()) {
            current = current->GetSubKeyByName(part);
            if (!current) return nullptr;
        }
        start = end + 1;
    }

    if (start < path.size()) {
        std::string part = path.substr(start);
        if (!part.empty()) {
            current = current->GetSubKeyByName(part);
        }
    }

    return current;
}

bool HiveParser::ReplayTransactionLogs(const std::vector<std::vector<uint8_t>>& logBuffers) {
    if (!isOpen_ || data_.empty()) {
        return false;
    }

    bool anyReplayed = false;

    for (const auto& logData : logBuffers) {
        if (logData.size() < sizeof(HiveBaseBlock)) {
            continue;
        }

        // Scan for HBIN blocks in the log and overlay onto main hive
        for (size_t pos = sizeof(HiveBaseBlock); pos + sizeof(HiveBinHeader) <= logData.size(); ) {
            uint32_t sig;
            std::memcpy(&sig, logData.data() + pos, sizeof(sig));

            if (sig != 0x6E696268) { // "hbin"
                ++pos;
                continue;
            }

            HiveBinHeader header;
            std::memcpy(&header, logData.data() + pos, sizeof(header));

            if (header.size == 0 || header.size > 0x10000000) {
                ++pos;
                continue;
            }

            size_t hivePos = hiveBinsOffset_ + header.offset;
            if (hivePos + header.size <= data_.size() && pos + header.size <= logData.size()) {
                std::memcpy(data_.data() + hivePos, logData.data() + pos, header.size);
                anyReplayed = true;
            }

            pos += header.size;
        }
    }

    if (anyReplayed) {
        // Re-parse after overlaying log data
        HiveKey oldRoot = std::move(rootKey_);
        rootKey_ = HiveKey();
        isOpen_ = false;
        if (!ParseHive()) {
            rootKey_ = std::move(oldRoot);
            isOpen_ = true;
            return false;
        }
        // Mark as clean after successful replay
        isDirty_ = false;
    }

    return anyReplayed;
}

bool HiveParser::KeyExists(const std::string& path) {
    return GetKeyByPath(path) != nullptr;
}

} // namespace amcache
