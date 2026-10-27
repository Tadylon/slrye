#include <platform/file_io.h>

#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace slrye::platform
{
namespace
{

void set_error(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}

}

bool read_text_file(
    const std::filesystem::path& path,
    std::string& contents,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        set_error(error, "Unable to open file for reading: " + path.string());
        return false;
    }

    contents.assign(
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>());
    if (!stream.good() && !stream.eof())
    {
        set_error(error, "Unable to read file: " + path.string());
        return false;
    }

    return true;
}

bool write_text_file_atomic(
    const std::filesystem::path& path,
    std::string_view contents,
    std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }

    if (path.empty())
    {
        set_error(error, "Cannot write an empty file path");
        return false;
    }

    std::error_code filesystem_error;
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, filesystem_error);
        if (filesystem_error)
        {
            set_error(error, "Unable to create directory " + parent.string() + ": " + filesystem_error.message());
            return false;
        }
    }

    std::filesystem::path temporary_path = path;
    temporary_path += ".tmp";

    std::ofstream stream(temporary_path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        set_error(error, "Unable to open file for writing: " + temporary_path.string());
        return false;
    }

    stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    stream.flush();
    const bool write_succeeded = static_cast<bool>(stream);
    stream.close();
    if (!write_succeeded || !stream)
    {
        std::filesystem::remove(temporary_path, filesystem_error);
        set_error(error, "Unable to write file: " + temporary_path.string());
        return false;
    }

#ifdef _WIN32
    if (MoveFileExW(
            temporary_path.c_str(),
            path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == FALSE)
    {
        const DWORD error_code = GetLastError();
        std::filesystem::remove(temporary_path, filesystem_error);
        set_error(
            error,
            "Unable to replace file " + path.string() + ": " +
                std::system_category().message(static_cast<int>(error_code)));
        return false;
    }
#else
    std::filesystem::rename(temporary_path, path, filesystem_error);
    if (filesystem_error)
    {
        std::filesystem::remove(temporary_path, filesystem_error);
        set_error(error, "Unable to replace file " + path.string() + ": " + filesystem_error.message());
        return false;
    }
#endif

    return true;
}

}
