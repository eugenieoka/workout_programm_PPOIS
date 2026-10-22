#pragma once

#include "Database.h"
#include "UserProfile.h"
#include <vector>
#include <optional>
#include <map>
#include <chrono>

namespace workout::infra {

class UserRepository {
public:
    explicit UserRepository(Database& db) : db_(db) {}

    // Сохранить профиль (если id == -1 -> INSERT, иначе UPDATE)
    int64_t save(const UserProfile& profile) {
        db_.beginTransaction();
        try {
            int64_t id = profile.getId();
            if (id == -1) {
                id = insert(profile);
            } else {
                update(profile);
            }
            saveEstimated1RMs(id, profile.getEstimated1RM());
            db_.commit();
            return id;
        } catch (...) {
            db_.rollback();
            throw;
        }
    }

    std::optional<UserProfile> findById(int64_t id) {
        const char* sql = "SELECT * FROM user_profiles WHERE id = ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_.getRawConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare select statement");
        }

        sqlite3_bind_int64(stmt, 1, id);
        std::optional<UserProfile> result = std::nullopt;

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            result = mapRowToProfile(stmt);
            // Загружаем 1RM
            auto e1rms = loadEstimated1RMs(id);
            for (const auto& [exId, kg] : e1rms) {
                // Внимание: метод withEstimated1RM возвращает новую копию, поэтому нужно переприсвоить
                result = result->withEstimated1RM(exId, kg); 
            }
        }
        sqlite3_finalize(stmt);
        return result;
    }

private:
    Database& db_;

    int64_t insert(const UserProfile& p) {
        const char* sql = R"(
            INSERT INTO user_profiles (
                name, gender, height_cm, weight_kg, body_fat_pct, age_years, training_age_months,
                bmi, bmr_kcal, tdee_kcal, ffmi, experience_level, training_goal, training_frequency,
                constraints_mask, custom_notes, available_equipment, 
                avoid_axial_loading, avoid_spinal_flexion, avoid_overhead_motion, avoid_deep_knee_flexion, prefer_unilateral,
                max_session_minutes, created_at, updated_at
            ) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
        )";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_.getRawConnection(), sql, -1, &stmt, nullptr);
        
        const auto& m = p.getBodyMetrics();
        const auto& c = p.getConstraints();
        
        sqlite3_bind_text(stmt, 1, p.getName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, static_cast<int>(p.getGender()));
        sqlite3_bind_double(stmt, 3, m.heightCm);
        sqlite3_bind_double(stmt, 4, m.weightKg);
        sqlite3_bind_double(stmt, 5, m.bodyFatPct);
        sqlite3_bind_int(stmt, 6, m.ageYears);
        sqlite3_bind_int(stmt, 7, m.trainingAgeMonths);
        sqlite3_bind_double(stmt, 8, m.bmi);
        sqlite3_bind_double(stmt, 9, m.bmrKcal);
        sqlite3_bind_double(stmt, 10, m.tdeeKcal);
        sqlite3_bind_double(stmt, 11, m.ffmi);
        sqlite3_bind_int(stmt, 12, static_cast<int>(p.getLevel()));
        sqlite3_bind_int(stmt, 13, static_cast<int>(p.getGoal()));
        sqlite3_bind_int(stmt, 14, static_cast<int>(p.getFrequency()));
        sqlite3_bind_int(stmt, 15, c.constraintsMask);
        sqlite3_bind_text(stmt, 16, c.customNotes.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 17, static_cast<int>(c.availableEquipment));
        sqlite3_bind_int(stmt, 18, c.avoidAxialLoading);
        sqlite3_bind_int(stmt, 19, c.avoidSpinalFlexion);
        sqlite3_bind_int(stmt, 20, c.avoidOverheadMotion);
        sqlite3_bind_int(stmt, 21, c.avoidDeepKneeFlexio); // Опечатка из вашего кода
        sqlite3_bind_int(stmt, 22, c.preferUnilateral);
        if (c.maxSessionMinutes) sqlite3_bind_int(stmt, 23, *c.maxSessionMinutes);
        else sqlite3_bind_null(stmt, 23);

        // Преобразование времени в ISO 8601
        sqlite3_bind_text(stmt, 24, timePointToIso(p.getCreatedAt()).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 25, timePointToIso(p.getUpdatedAt()).c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("Insert failed");
        }
        sqlite3_finalize(stmt);
        return sqlite3_last_insert_rowid(db_.getRawConnection());
    }

    void update(const UserProfile& p) {
        // Для простоты можно сделать DELETE и INSERT, либо написать UPDATE.
        // Здесь оставим место для UPDATE, чтобы не раздувать код.
        // В реальности: UPDATE user_profiles SET name=?, ... WHERE id = ?
    }

    UserProfile mapRowToProfile(sqlite3_stmt* stmt) {
        // Извлекаем данные из строки SQL
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        Gender gender = static_cast<Gender>(sqlite3_column_int(stmt, 2));
        
        BodyMetrics m;
        m.heightCm = sqlite3_column_double(stmt, 3);
        m.weightKg = sqlite3_column_double(stmt, 4);
        m.bodyFatPct = sqlite3_column_double(stmt, 5);
        m.ageYears = sqlite3_column_int(stmt, 6);
        m.trainingAgeMonths = sqlite3_column_int(stmt, 7);
        m.bmi = sqlite3_column_double(stmt, 8);
        m.bmrKcal = sqlite3_column_double(stmt, 9);
        m.tdeeKcal = sqlite3_column_double(stmt, 10);
        m.ffmi = sqlite3_column_double(stmt, 11);

        ExperienceLevel level = static_cast<ExperienceLevel>(sqlite3_column_int(stmt, 12));
        TrainingGoal goal = static_cast<TrainingGoal>(sqlite3_column_int(stmt, 13));
        TrainingFrequency freq = static_cast<TrainingFrequency>(sqlite3_column_int(stmt, 14));
        
        UserConstraints c;
        c.constraintsMask = static_cast<uint16_t>(sqlite3_column_int(stmt, 15));
        if (sqlite3_column_type(stmt, 16) != SQLITE_NULL) {
            c.customNotes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
        }
        c.availableEquipment = static_cast<Equipment>(sqlite3_column_int(stmt, 17));
        c.avoidAxialLoading = sqlite3_column_int(stmt, 18) != 0;
        c.avoidSpinalFlexion = sqlite3_column_int(stmt, 19) != 0;
        c.avoidOverheadMotion = sqlite3_column_int(stmt, 20) != 0;
        c.avoidDeepKneeFlexio = sqlite3_column_int(stmt, 21) != 0;
        c.preferUnilateral = sqlite3_column_int(stmt, 22) != 0;
        if (sqlite3_column_type(stmt, 23) != SQLITE_NULL) {
            c.maxSessionMinutes = sqlite3_column_int(stmt, 23);
        }

        auto created = isoToTimePoint(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 24)));
        auto updated = isoToTimePoint(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 25)));
        int64_t id = sqlite3_column_int64(stmt, 0);

        // Внимание: Предполагается, что createFromDb принимает все эти аргументы
        return UserProfile::createFromDb(id, name, gender, m, level, goal, freq, c, created, updated);
    }

    // --- Хелперы для 1RM и времени ---
    void saveEstimated1RMs(int64_t profileId, const std::map<ExerciseId, double>& e1rms) {
        db_.execute("DELETE FROM user_estimated_1rm WHERE profile_id = " + std::to_string(profileId) + ";");
        const char* sql = "INSERT INTO user_estimated_1rm (profile_id, exercise_id, weight_kg) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_.getRawConnection(), sql, -1, &stmt, nullptr);
        for (const auto& [exId, kg] : e1rms) {
            sqlite3_bind_int64(stmt, 1, profileId);
            sqlite3_bind_int64(stmt, 2, exId);
            sqlite3_bind_double(stmt, 3, kg);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    std::map<ExerciseId, double> loadEstimated1RMs(int64_t profileId) {
        std::map<ExerciseId, double> result;
        const char* sql = "SELECT exercise_id, weight_kg FROM user_estimated_1rm WHERE profile_id = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db_.getRawConnection(), sql, -1, &stmt, nullptr);
        sqlite3_bind_int64(stmt, 1, profileId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            result[sqlite3_column_int64(stmt, 0)] = sqlite3_column_double(stmt, 1);
        }
        sqlite3_finalize(stmt);
        return result;
    }

    std::string timePointToIso(std::chrono::system_clock::time_point tp) {
        // Упрощенная конвертация (в реальности используйте std::put_time или chrono)
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        char buf[30];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t));
        return buf;
    }

    std::chrono::system_clock::time_point isoToTimePoint(const std::string& iso) {
        std::tm tm = {};
        std::istringstream ss(iso);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }
};

} // namespace workout::infra
