//
// Created by hackerman on 1/28/24.
//

#pragma once
#include "common.hpp"


class CWatch
{
public:
    struct Hasher
    {
        size_t operator()(const CWatch& owner) const;
    };
protected:
    CWatch(int32_t fd, std::string path, uint32_t eventMask);
public:
    [[nodiscard]] static std::expected<CWatch, common::ErrorCode> make_watch(int32_t fd, std::string path, uint32_t eventMask);
    ~CWatch();
    CWatch(const CWatch& other) = delete;
    CWatch(CWatch&& other) noexcept;
    CWatch& operator=(const CWatch& other) = delete;
    CWatch& operator=(CWatch&& other) noexcept;
public:
    friend bool operator==(const CWatch& lhs, const CWatch& rhs);
    [[nodiscard]] size_t operator()(const CWatch& self) const;
    [[nodiscard]] std::string_view watching() const;
private:
    int32_t m_FileDescriptor;
    int32_t m_WatchDescriptor;
    uint32_t m_EventMask;
    std::string m_WatchedFile;
};