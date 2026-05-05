#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <cstdint>
#include "HiveParser.h"

namespace amcache {

class RegistryValue {
public:
    enum class Type {
        None = 0,
        String = 1,
        ExpandString = 2,
        Binary = 3,
        DWord = 4,
        DWordBigEndian = 5,
        Link = 6,
        MultiString = 7,
        ResourceList = 8,
        FullResourceDescriptor = 9,
        ResourceRequirementsList = 10,
        QWord = 11
    };

    RegistryValue() = default;
    explicit RegistryValue(HiveValue* value) : value_(value) {}

    bool IsValid() const { return value_ != nullptr && value_->IsValid(); }
    std::string GetName() const { return value_ ? value_->GetName() : ""; }
    Type GetType() const {
        return value_ ? static_cast<Type>(value_->GetType()) : Type::None;
    }

    std::optional<std::string> GetString() const {
        return value_ ? value_->GetString() : std::nullopt;
    }
    std::optional<int32_t> GetInt32() const {
        return value_ ? value_->GetInt32() : std::nullopt;
    }
    std::optional<uint32_t> GetUInt32() const {
        return value_ ? value_->GetUInt32() : std::nullopt;
    }
    std::optional<int64_t> GetInt64() const {
        return value_ ? value_->GetInt64() : std::nullopt;
    }
    std::optional<uint64_t> GetUInt64() const {
        return value_ ? value_->GetUInt64() : std::nullopt;
    }
    std::optional<std::vector<uint8_t>> GetBinary() const {
        return value_ ? value_->GetBinary() : std::nullopt;
    }

private:
    HiveValue* value_ = nullptr;
};

class RegistryKey {
public:
    RegistryKey() = default;
    explicit RegistryKey(HiveKey* key) : key_(key) {}

    bool IsValid() const { return key_ != nullptr && key_->IsValid(); }
    std::string GetName() const { return key_ ? key_->GetName() : ""; }
    std::chrono::system_clock::time_point GetLastWriteTime() const {
        return key_ ? key_->GetLastWriteTime() : std::chrono::system_clock::time_point{};
    }

    int GetSubKeyCount() const { return key_ ? key_->GetSubKeyCount() : 0; }

    RegistryKey GetSubKey(int index) const {
        if (!key_ || index < 0 || index >= key_->GetSubKeyCount()) {
            return RegistryKey();
        }
        return RegistryKey(const_cast<HiveKey*>(&key_->GetSubKeys()[index]));
    }

    RegistryKey GetSubKeyByName(const std::string& name) const {
        return key_ ? RegistryKey(key_->GetSubKeyByName(name)) : RegistryKey();
    }

    std::vector<RegistryKey> GetSubKeys() const {
        std::vector<RegistryKey> result;
        if (key_) {
            const auto& subkeys = key_->GetSubKeys();
            result.reserve(subkeys.size());
            for (const auto& sk : subkeys) {
                result.emplace_back(const_cast<HiveKey*>(&sk));
            }
        }
        return result;
    }

    int GetValueCount() const { return key_ ? key_->GetValueCount() : 0; }

    RegistryValue GetValue(int index) const {
        if (!key_ || index < 0 || index >= key_->GetValueCount()) {
            return RegistryValue();
        }
        return RegistryValue(const_cast<HiveValue*>(&key_->GetValues()[index]));
    }

    RegistryValue GetValueByName(const std::string& name) const {
        return key_ ? RegistryValue(key_->GetValueByName(name)) : RegistryValue();
    }

    std::vector<RegistryValue> GetValues() const {
        std::vector<RegistryValue> result;
        if (key_) {
            const auto& values = key_->GetValues();
            result.reserve(values.size());
            for (const auto& v : values) {
                result.emplace_back(const_cast<HiveValue*>(&v));
            }
        }
        return result;
    }

    std::optional<std::string> GetStringValue(const std::string& name) const {
        return key_ ? key_->GetStringValue(name) : std::nullopt;
    }
    std::optional<int32_t> GetInt32Value(const std::string& name) const {
        return key_ ? key_->GetInt32Value(name) : std::nullopt;
    }
    std::optional<uint32_t> GetUInt32Value(const std::string& name) const {
        return key_ ? key_->GetUInt32Value(name) : std::nullopt;
    }
    std::optional<int64_t> GetInt64Value(const std::string& name) const {
        return key_ ? key_->GetInt64Value(name) : std::nullopt;
    }
    std::optional<uint64_t> GetUInt64Value(const std::string& name) const {
        return key_ ? key_->GetUInt64Value(name) : std::nullopt;
    }
    std::optional<std::vector<uint8_t>> GetBinaryValue(const std::string& name) const {
        return key_ ? key_->GetBinaryValue(name) : std::nullopt;
    }

private:
    HiveKey* key_ = nullptr;
};

class RegistryHive {
public:
    RegistryHive() : parser_(std::make_unique<HiveParser>()) {}
    ~RegistryHive() = default;

    RegistryHive(RegistryHive&& other) noexcept = default;
    RegistryHive& operator=(RegistryHive&& other) noexcept = default;

    RegistryHive(const RegistryHive&) = delete;
    RegistryHive& operator=(const RegistryHive&) = delete;

    bool Open(const std::string& path) {
        return parser_->Open(path);
    }

    bool OpenFromBuffer(const std::vector<uint8_t>& buffer) {
        return parser_->OpenFromBuffer(buffer);
    }

    void Close() {
        parser_->Close();
    }

    bool IsOpen() const { return parser_->IsOpen(); }

    RegistryKey GetRootKey() const {
        return RegistryKey(parser_->GetRootKey());
    }

    RegistryKey GetKeyByPath(const std::string& path) const {
        return RegistryKey(parser_->GetKeyByPath(path));
    }

    bool KeyExists(const std::string& path) const {
        return parser_->KeyExists(path);
    }

    bool IsDirty() const {
        return parser_->IsDirty();
    }

    uint32_t GetPrimarySequence() const { return parser_->GetPrimarySequence(); }
    uint32_t GetSecondarySequence() const { return parser_->GetSecondarySequence(); }

private:
    std::unique_ptr<HiveParser> parser_;
};

} // namespace amcache
