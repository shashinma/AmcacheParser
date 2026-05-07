#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>

#ifdef _WIN32
#undef REG_NONE
#undef REG_SZ
#undef REG_EXPAND_SZ
#undef REG_BINARY
#undef REG_DWORD
#undef REG_DWORD_BIG_ENDIAN
#undef REG_LINK
#undef REG_MULTI_SZ
#undef REG_RESOURCE_LIST
#undef REG_FULL_RESOURCE_DESCRIPTOR
#undef REG_RESOURCE_REQUIREMENTS_LIST
#undef REG_QWORD
#endif

namespace amcache {

enum class RegValueType : uint32_t {
    REG_NONE = 0,
    REG_SZ = 1,
    REG_EXPAND_SZ = 2,
    REG_BINARY = 3,
    REG_DWORD = 4,
    REG_DWORD_BIG_ENDIAN = 5,
    REG_LINK = 6,
    REG_MULTI_SZ = 7,
    REG_RESOURCE_LIST = 8,
    REG_FULL_RESOURCE_DESCRIPTOR = 9,
    REG_RESOURCE_REQUIREMENTS_LIST = 10,
    REG_QWORD = 11
};

#pragma pack(push, 1)

struct HiveBaseBlock {
    uint32_t signature;
    uint32_t primarySequence;
    uint32_t secondarySequence;
    uint64_t lastWriteTime;
    uint32_t majorVersion;
    uint32_t minorVersion;
    uint32_t type;
    uint32_t format;
    uint32_t rootCellOffset;
    uint32_t hiveBinsDataSize;
    uint32_t clusteringFactor;
    char fileName[64];
    uint8_t reserved1[396];
    uint32_t checksum;
    uint8_t reserved2[3576];
    uint32_t bootType;
    uint32_t bootRecover;
};

struct HiveBinHeader {
    uint32_t signature;
    uint32_t offset;
    uint32_t size;
    uint64_t reserved1;
    uint64_t timestamp;
    uint32_t spare;
};

struct KeyNode {
    uint16_t signature;
    uint16_t flags;
    uint64_t lastWriteTime;
    uint32_t accessBits;
    uint32_t parentOffset;
    uint32_t subKeyCount;
    uint32_t volatileSubKeyCount;
    uint32_t subKeyListOffset;
    uint32_t volatileSubKeyListOffset;
    uint32_t valueCount;
    uint32_t valueListOffset;
    uint32_t securityOffset;
    uint32_t classNameOffset;
    uint32_t maxSubKeyNameLen;
    uint32_t maxSubKeyClassLen;
    uint32_t maxValueNameLen;
    uint32_t maxValueDataLen;
    uint32_t workVar;
    uint16_t keyNameLen;
    uint16_t classNameLen;
};

struct ValueNode {
    uint16_t signature;
    uint16_t nameLen;
    uint32_t dataSize;
    uint32_t dataOffset;
    uint32_t dataType;
    uint16_t flags;
    uint16_t spare;
};

#pragma pack(pop)

class HiveValue {
public:
    HiveValue() = default;

    std::string GetName() const { return name_; }
    RegValueType GetType() const { return type_; }

    std::optional<std::string> GetString() const;
    std::optional<int32_t> GetInt32() const;
    std::optional<uint32_t> GetUInt32() const;
    std::optional<int64_t> GetInt64() const;
    std::optional<uint64_t> GetUInt64() const;
    std::optional<std::vector<uint8_t>> GetBinary() const;

    bool IsValid() const { return valid_; }

private:
    friend class HiveParser;

    std::string name_;
    RegValueType type_ = RegValueType::REG_NONE;
    std::vector<uint8_t> data_;
    bool valid_ = false;
};

class HiveKey {
public:
    HiveKey() = default;

    std::string GetName() const { return name_; }
    std::chrono::system_clock::time_point GetLastWriteTime() const { return lastWriteTime_; }

    int GetSubKeyCount() const { return static_cast<int>(subKeys_.size()); }
    const HiveKey& GetSubKey(int index) const { return subKeys_[index]; }
    HiveKey* GetSubKeyByName(const std::string& name);
    const std::vector<HiveKey>& GetSubKeys() const { return subKeys_; }

    int GetValueCount() const { return static_cast<int>(values_.size()); }
    const HiveValue& GetValue(int index) const { return values_[index]; }
    HiveValue* GetValueByName(const std::string& name);
    const std::vector<HiveValue>& GetValues() const { return values_; }

    std::optional<std::string> GetStringValue(const std::string& name) const;
    std::optional<int32_t> GetInt32Value(const std::string& name) const;
    std::optional<uint32_t> GetUInt32Value(const std::string& name) const;
    std::optional<int64_t> GetInt64Value(const std::string& name) const;
    std::optional<uint64_t> GetUInt64Value(const std::string& name) const;
    std::optional<std::vector<uint8_t>> GetBinaryValue(const std::string& name) const;

    bool IsValid() const { return valid_; }

private:
    friend class HiveParser;

    std::string name_;
    std::chrono::system_clock::time_point lastWriteTime_;
    std::vector<HiveKey> subKeys_;
    std::vector<HiveValue> values_;
    bool valid_ = false;
};

class HiveParser {
public:
    HiveParser() = default;
    ~HiveParser() = default;

    bool Open(const std::string& path);
    bool OpenFromBuffer(const std::vector<uint8_t>& buffer);
    void Close();
    void SetRecoverDeleted(bool value) { recoverDeleted_ = value; }

    bool IsOpen() const { return isOpen_; }
    HiveKey* GetRootKey() { return &rootKey_; }
    HiveKey* GetKeyByPath(const std::string& path);
    bool KeyExists(const std::string& path);
    bool IsDirty() const { return isDirty_; }

    uint32_t GetPrimarySequence() const { return primarySequence_; }
    uint32_t GetSecondarySequence() const { return secondarySequence_; }
    bool ReplayTransactionLogs(const std::vector<std::vector<uint8_t>>& logBuffers);

private:
    bool ParseHive();
    bool ParseKeyNode(uint32_t offset, HiveKey& key);
    bool ParseSubKeys(uint32_t listOffset, uint32_t count, HiveKey& parentKey);
    bool ParseValues(uint32_t listOffset, uint32_t count, HiveKey& key);
    bool ParseValue(uint32_t offset, HiveValue& value);

    template<typename T>
    bool ReadAt(uint32_t offset, T& value);
    bool ReadBytes(uint32_t offset, void* buffer, size_t size);
    std::string ReadString(uint32_t offset, uint32_t length, bool isUtf16 = false);

    static std::chrono::system_clock::time_point FileTimeToTimePoint(uint64_t filetime);

    std::vector<uint8_t> data_;
    HiveKey rootKey_;
    bool isOpen_ = false;
    bool isDirty_ = false;
    uint32_t primarySequence_ = 0;
    uint32_t secondarySequence_ = 0;
    uint32_t hiveBinsOffset_ = 4096;
    bool recoverDeleted_ = true;
};

} // namespace amcache
