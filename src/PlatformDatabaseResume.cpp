#include "PlatformDatabase.h"
#include <winsqlite/winsqlite3.h>

namespace zero {

bool PlatformDatabase::ClearResume(const std::string& packageId, std::wstring& error) const {
    if (!Initialize(error)) return false;

    sqlite3* db = nullptr;
    if (sqlite3_open16(databasePath_.c_str(), &db) != SQLITE_OK) {
        if (db) sqlite3_close(db);
        error = L"ZERO could not open the platform database to clear Resume metadata.";
        return false;
    }
    sqlite3_busy_timeout(db, 3000);
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM resume_activities WHERE package_id=?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        error = L"ZERO could not prepare the Resume database delete.";
        return false;
    }
    const bool bound = sqlite3_bind_text(stmt, 1, packageId.c_str(), static_cast<int>(packageId.size()), SQLITE_TRANSIENT) == SQLITE_OK;
    const bool done = bound && sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if (!done) {
        error = L"ZERO could not clear Resume metadata from the platform database.";
        return false;
    }
    return true;
}

} // namespace zero
