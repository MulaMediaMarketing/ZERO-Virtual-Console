#include "PackageManifestParser.h"
#include "ManifestValidator.h"
#include "StrictJson.h"
#include "Utf8Path.h"
#include <fstream>
#include <limits>
#include <sstream>

namespace zero {
namespace {
constexpr size_t kMaxManifestBytes = 256 * 1024;

std::string readAllBounded(const std::filesystem::path& path, std::wstring& error) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size > kMaxManifestBytes) { error = L"ZERO manifest is missing, unreadable, or exceeds the supported size."; return {}; }
    std::ifstream file(path, std::ios::binary);
    if (!file) { error = L"ZERO could not open the package manifest."; return {}; }
    std::ostringstream stream; stream << file.rdbuf(); return stream.str();
}
bool requiredString(const json::Object& object,std::string_view key,std::string& out,size_t maxBytes,std::wstring& error){const auto* value=json::String(object,key);if(!value||value->empty()||value->size()>maxBytes){error=L"ZERO manifest is missing a required string field or the field is too large.";return false;}out=*value;return true;}
bool optionalString(const json::Object& object,std::string_view key,std::string& out,size_t maxBytes,std::wstring& error){const auto* raw=json::Find(object,key);if(!raw){out.clear();return true;}const auto* value=raw->AsString();if(!value||value->size()>maxBytes){error=L"ZERO manifest contains an invalid optional string field.";return false;}out=*value;return true;}
bool optionalBool(const json::Object& object,std::string_view key,bool fallback,bool& out,std::wstring& error){const auto* raw=json::Find(object,key);if(!raw){out=fallback;return true;}const auto* value=raw->AsBool();if(!value){error=L"ZERO manifest contains a non-boolean feature flag.";return false;}out=*value;return true;}
bool requiredInt(const json::Object& object,std::string_view key,int fallback,int& out,std::wstring& error){const auto* raw=json::Find(object,key);if(!raw){out=fallback;return true;}const auto* value=raw->AsInt();if(!value||*value<std::numeric_limits<int>::min()||*value>std::numeric_limits<int>::max()){error=L"ZERO manifest contains an invalid integer field.";return false;}out=static_cast<int>(*value);return true;}
bool packagePath(const std::filesystem::path& root,const std::string& relative,std::filesystem::path& out,std::wstring& error){if(relative.empty()){out=std::filesystem::path{};return true;}const auto converted=PathFromUtf8(relative);if(!converted){error=L"ZERO manifest contains an invalid UTF-8 filesystem path.";return false;}out=root / *converted;return true;}
}

PackageManifestParseResult PackageManifestParser::ParseFile(const std::filesystem::path& manifestPath,const std::filesystem::path& packageRoot){
    PackageManifestParseResult result;auto text=readAllBounded(manifestPath,result.error);if(text.empty())return result;
    const auto parsed=json::Parse(text);if(!parsed.ok){result.error=L"ZERO manifest contains invalid JSON.";return result;}
    const auto* object=parsed.root.AsObject();if(!object){result.error=L"ZERO manifest root must be a JSON object.";return result;}
    GameManifest manifest;manifest.root=packageRoot;
    if(!requiredInt(*object,"schema",1,manifest.schemaVersion,result.error)||!requiredInt(*object,"minimum_runtime_major",4,manifest.minimumRuntimeMajor,result.error)||!requiredString(*object,"package_id",manifest.packageId,160,result.error)||!requiredString(*object,"title",manifest.title,512,result.error)||!requiredString(*object,"version",manifest.version,128,result.error))return result;
    std::string executable,hero,icon,logo;
    if(!requiredString(*object,"executable",executable,2048,result.error)||!optionalString(*object,"hero_image",hero,2048,result.error)||!optionalString(*object,"icon_image",icon,2048,result.error)||!optionalString(*object,"logo_image",logo,2048,result.error)||!optionalBool(*object,"zero_resume",false,manifest.zeroResume,result.error)||!optionalBool(*object,"zero_achievements",false,manifest.zeroAchievements,result.error)||!optionalBool(*object,"zero_overlay",true,manifest.zeroOverlay,result.error)||!optionalBool(*object,"zero_input",true,manifest.zeroInput,result.error))return result;
    if(!packagePath(packageRoot,executable,manifest.executable,result.error)||!packagePath(packageRoot,hero,manifest.heroImage,result.error)||!packagePath(packageRoot,icon,manifest.iconImage,result.error)||!packagePath(packageRoot,logo,manifest.logoImage,result.error))return result;
    const auto validation=ManifestValidator::Validate(manifest);if(!validation.valid){result.error=validation.error.empty()?L"ZERO manifest failed schema validation.":validation.error;return result;}
    result.manifest=std::move(manifest);result.valid=true;return result;
}

} // namespace zero