// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Marcos Quirino Deida
#pragma once

// Not JUCE. Source/Plugin/HQRenderer.cpp is shared with the JUCE build and uses a handful of juce_core
// classes to find, copy and load the simulator library. The OpenPluginSDK build has no JUCE, so this header
// stands in for <juce_core/juce_core.h> with just those calls, written on the C++ standard library and
// dlopen. Nothing here is taken from JUCE.

#include <cstdlib>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#if defined(__APPLE__)
 #define JUCE_MAC 1
#elif defined(__linux__)
 #define JUCE_LINUX 1
#endif
#if defined(_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX // or <windows.h> turns std::max into a macro call
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
#else
 #include <dlfcn.h>
#endif

namespace juce
{
template <typename... T> void ignoreUnused(T&&...) noexcept {}

class String
{
public:
    String() = default;
    String(const char* s) : text(s ? s : "") {}
    String(std::string s) : text(std::move(s)) {}
    static String fromUTF8(const char* s) { return String(s); }
    String toLowerCase() const
    {
        std::string out = text;
        for (auto& ch : out) if (ch >= 'A' && ch <= 'Z') ch = char(ch - 'A' + 'a');
        return out;
    }
    const std::string& toStdString() const { return text; }
    bool isNotEmpty() const { return !text.empty(); }
    friend String operator+(const char* a, const String& b) { return String(std::string(a) + b.text); }

private:
    std::string text;
};

class StringArray
{
public:
    void addLines(const String& all)
    {
        std::istringstream in(all.toStdString());
        for (std::string line; std::getline(in, line);)
        {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.emplace_back(line);
        }
    }
    auto begin() const { return lines.begin(); }
    auto end() const { return lines.end(); }

private:
    std::vector<String> lines;
};

struct Uuid
{
    String toString() const
    {
        std::random_device device;
        std::ostringstream out;
        out << std::hex;
        for (int i = 0; i < 4; ++i) out << device();
        return out.str();
    }
};

struct SystemStats
{
    static String getEnvironmentVariable(const char* name, const char* fallback)
    {
        const char* value = std::getenv(name);
        return String(value && *value ? value : fallback);
    }
};

class File
{
public:
    enum SpecialLocationType { tempDirectory };
    File() = default;
    File(const String& p) : path(p.toStdString()) {}
    File(const char* p) : path(p) {}
    static File getSpecialLocation(SpecialLocationType) { return at(std::filesystem::temp_directory_path(ignored())); }
    File getParentDirectory() const { return at(path.parent_path()); }
    File getChildFile(const char* name) const { return at(path / name); }
    bool existsAsFile() const { return !path.empty() && std::filesystem::is_regular_file(path, ignored()); }
    bool isSymbolicLink() const { return std::filesystem::is_symlink(path, ignored()); }
    File getLinkedTarget() const
    {
        auto target = std::filesystem::read_symlink(path, ignored());
        return at(target.is_absolute() ? target : path.parent_path() / target);
    }
    String getFileExtension() const { return path.extension().string(); }
    File getNonexistentChildFile(const String& stem, const String& extension) const
    {
        auto candidate = path / (stem.toStdString() + extension.toStdString());
        for (int n = 2; std::filesystem::exists(candidate, ignored()); ++n)
            candidate = path / (stem.toStdString() + " (" + std::to_string(n) + ")" + extension.toStdString());
        return at(candidate);
    }
    bool copyFileTo(const File& target) const
    {
        std::error_code error;
        return std::filesystem::copy_file(path, target.path, std::filesystem::copy_options::overwrite_existing, error) && !error;
    }
    bool deleteFile() const { return std::filesystem::remove(path, ignored()); }
    String getFullPathName() const { return path.string(); }

private:
    std::filesystem::path path;
    static File at(std::filesystem::path p) { File f; f.path = std::move(p); return f; }
    static std::error_code& ignored() { static thread_local std::error_code e; return e; }
};

class DynamicLibrary
{
public:
    ~DynamicLibrary() { close(); }
    bool open(const String& name)
    {
        close();
#if defined(_WIN32)
        handle = LoadLibraryA(name.toStdString().c_str());
#else
        handle = dlopen(name.toStdString().c_str(), RTLD_LOCAL | RTLD_NOW);
#endif
        return handle != nullptr;
    }
    void close()
    {
        if (!handle) return;
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
        handle = nullptr;
    }
    void* getFunction(const char* name) const
    {
        if (!handle) return nullptr;
#if defined(_WIN32)
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#else
        return dlsym(handle, name);
#endif
    }

private:
    void* handle = nullptr;
};
} // namespace juce
