#include <vengine/core/Error.hpp>

#include <cstdio>

namespace vengine::core {

namespace {
struct CodeName { ErrorCode code; const char* name; };
constexpr CodeName kCodeNames[] = {
    {ErrorCode::Ok,                    "Ok"},
    {ErrorCode::Unknown,               "Unknown"},
    {ErrorCode::InvalidArgument,       "InvalidArgument"},
    {ErrorCode::InvalidState,          "InvalidState"},
    {ErrorCode::NotSupported,          "NotSupported"},
    {ErrorCode::Cancelled,             "Cancelled"},
    {ErrorCode::Timeout,               "Timeout"},
    {ErrorCode::FileNotFound,          "FileNotFound"},
    {ErrorCode::PermissionDenied,      "PermissionDenied"},
    {ErrorCode::IoFailure,             "IoFailure"},
    {ErrorCode::AssetNotFound,         "AssetNotFound"},
    {ErrorCode::AssetCorrupted,        "AssetCorrupted"},
    {ErrorCode::UnsupportedFormat,     "UnsupportedFormat"},
    {ErrorCode::ShaderCompilationFailed,"ShaderCompilationFailed"},
    {ErrorCode::TextureCreationFailed, "TextureCreationFailed"},
    {ErrorCode::GpuContextLost,        "GpuContextLost"},
    {ErrorCode::SerializationFailed,   "SerializationFailed"},
    {ErrorCode::DeserializationFailed, "DeserializationFailed"},
    {ErrorCode::InvalidScene,          "InvalidScene"},
    {ErrorCode::InvalidComponent,      "InvalidComponent"},
    {ErrorCode::OutOfMemory,           "OutOfMemory"},
    {ErrorCode::BuildFailed,           "BuildFailed"},
};
} // namespace

const char* error_code_name(ErrorCode code) noexcept {
    for (const auto& c : kCodeNames) {
        if (c.code == code) return c.name;
    }
    return "Unknown";
}

std::string Error::format() const {
    std::string out = error_code_name(code);
    if (!message.empty()) {
        out += ": ";
        out += message;
    }
    if (!detail.empty()) {
        out += " (";
        out += detail;
        out += ")";
    }
    return out;
}

} // namespace vengine::core
