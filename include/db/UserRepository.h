#pragma once

#include "Database.h"
#include "SqlTime.h"
#include "core/User.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace workout::infra {

class UserRepository {
public:
    explicit UserRepository(Database& db) : db_(db) {}

    // Save a profile: id == -1 -> INSERT, otherwise UPDATE.
    int64_t save(const core::UserProfile& profile) {
        db_.beginTransaction();
        try {
            const int64_t id = profile.getId() == -1 ? insert(profile) : update(profile);
            saveEstimated1RMs(id, profile.getEstimated1RM());
            db_.commit();
            return id;
        } catch (...) {
            try {
                db_.rollback();
            } catch (...) {
                // Preserve the original exception.
            }
            throw;
        }
    }

    std::optional<core::UserProfile> findById(int64_t id) {
        Statement stmt(db_.getRawConnection(), selectSql() + " WHERE id = ?;");
        stmt.bind(1, id);
        if (!stmt.step()) {
            return std::nullopt;
        }
        return mapRowToProfile(stmt, loadEstimated1RMs(id));
    }

    std::vector<core::UserProfile> findAll() {
        Statement stmt(db_.getRawConnection(), selectSql() + " ORDER BY id;");
        const auto oneRepMaxes = loadAllEstimated1RMs();
        const std::map<core::ExerciseId, double> none;

        std::vector<core::UserProfile> result;
        while (stmt.step()) {
            const auto id = stmt.columnInt64(0);
            const auto it = oneRepMaxes.find(id);
            result.push_back(mapRowToProfile(stmt, it == oneRepMaxes.end() ? none : it->second));
        }
        return result;
    }

    // @return true if the profile existed and was deleted.
    bool remove(int64_t id) {
        Statement stmt(db_.getRawConnection(), "DELETE FROM user_profiles WHERE id = ?;");
        stmt.bind(1, id);
        stmt.exec();
        return db_.changes() > 0;
    }

private:
    Database& db_;

    static constexpr const char* kColumns =
        "id, name, gender, height_cm, weight_kg, body_fat_pct, age_years, training_age_months, "
        "bmi, bmr_kcal, tdee_kcal, ffmi, experience_level, training_goal, training_frequency, "
        "constraints_mask, custom_notes, available_equipment, "
        "avoid_axial_loading, avoid_spinal_flexion, avoid_overhead_motion, avoid_deep_knee_flexion, "
        "prefer_unilateral, max_session_minutes, created_at, updated_at";

    static std::string selectSql() {
        return std::string("SELECT ") + kColumns + " FROM user_profiles";
    }

    int64_t insert(const core::UserProfile& p) {
        Statement stmt(db_.getRawConnection(), R"(
            INSERT INTO user_profiles (
                name, gender, height_cm, weight_kg, body_fat_pct, age_years, training_age_months,
                bmi, bmr_kcal, tdee_kcal, ffmi, experience_level, training_goal, training_frequency,
                constraints_mask, custom_notes, available_equipment,
                avoid_axial_loading, avoid_spinal_flexion, avoid_overhead_motion, avoid_deep_knee_flexion,
                prefer_unilateral, max_session_minutes, created_at, updated_at
            ) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
        )");
        bindProfileColumns(stmt, p);
        stmt.exec();
        return db_.lastInsertRowId();
    }

    int64_t update(const core::UserProfile& p) {
        Statement stmt(db_.getRawConnection(), R"(
            UPDATE user_profiles SET
                name = ?, gender = ?, height_cm = ?, weight_kg = ?, body_fat_pct = ?,
                age_years = ?, training_age_months = ?, bmi = ?, bmr_kcal = ?, tdee_kcal = ?,
                ffmi = ?, experience_level = ?, training_goal = ?, training_frequency = ?,
                constraints_mask = ?, custom_notes = ?, available_equipment = ?,
                avoid_axial_loading = ?, avoid_spinal_flexion = ?, avoid_overhead_motion = ?,
                avoid_deep_knee_flexion = ?, prefer_unilateral = ?, max_session_minutes = ?,
                created_at = ?, updated_at = ?
            WHERE id = ?;
        )");
        bindProfileColumns(stmt, p);
        stmt.bind(26, p.getId());
        stmt.exec();
        if (db_.changes() == 0) {
            throw std::runtime_error("UserProfile with id " + std::to_string(p.getId()) + " not found");
        }
        return p.getId();
    }

    static void bindProfileColumns(Statement& stmt, const core::UserProfile& p) {
        const auto& m = p.getBodyMetrics();
        const auto& c = p.getConstraints();

        stmt.bind(1, p.getName());
        stmt.bind(2, static_cast<int>(p.getGender()));
        stmt.bind(3, m.heightCm);
        stmt.bind(4, m.weightKg);
        stmt.bind(5, m.bodyFatPct);
        stmt.bind(6, m.ageYears);
        stmt.bind(7, m.trainingAgeMonths);
        stmt.bind(8, m.bmi);
        stmt.bind(9, m.bmrKcal);
        stmt.bind(10, m.tdeeKcal);
        stmt.bind(11, m.ffmi);
        stmt.bind(12, static_cast<int>(p.getLevel()));
        stmt.bind(13, static_cast<int>(p.getGoal()));
        stmt.bind(14, static_cast<int>(p.getFrequency()));
        stmt.bind(15, static_cast<int>(c.constraintsMask));

        if (c.customNotes.empty()) {
            stmt.bindNull(16);
        } else {
            stmt.bind(16, c.customNotes);
        }

        stmt.bind(17, static_cast<int>(static_cast<uint16_t>(c.availableEquipment)));
        stmt.bind(18, c.userAvoidAxialLoading ? 1 : 0);
        stmt.bind(19, c.userAvoidSpinalFlexion ? 1 : 0);
        stmt.bind(20, c.userAvoidOverheadMotion ? 1 : 0);
        stmt.bind(21, c.userAvoidDeepKneeFlexion ? 1 : 0);
        stmt.bind(22, c.userPreferUnilateral ? 1 : 0);

        if (c.maxSessionMinutes) {
            stmt.bind(23, *c.maxSessionMinutes);
        } else {
            stmt.bindNull(23);
        }

        stmt.bind(24, sql::toIso(p.getCreatedAt()));
        stmt.bind(25, sql::toIso(p.getUpdatedAt()));
    }

    core::UserProfile mapRowToProfile(const Statement& stmt,
                                     const std::map<core::ExerciseId, double>& estimated1RM) {
        const int64_t id = stmt.columnInt64(0);
        const std::string name = stmt.columnText(1);
        const auto gender = static_cast<core::Gender>(stmt.columnInt(2));

        core::BodyMetrics m;
        m.heightCm = stmt.columnDouble(3);
        m.weightKg = stmt.columnDouble(4);
        m.bodyFatPct = stmt.columnIsNull(5) ? -1.0 : stmt.columnDouble(5);
        m.ageYears = stmt.columnInt(6);
        m.trainingAgeMonths = stmt.columnInt(7);
        m.bmi = stmt.columnDouble(8);
        m.bmrKcal = stmt.columnDouble(9);
        m.tdeeKcal = stmt.columnDouble(10);
        m.ffmi = stmt.columnDouble(11);

        const auto level = static_cast<core::ExperienceLevel>(stmt.columnInt(12));
        const auto goal = static_cast<core::TrainingGoal>(stmt.columnInt(13));
        const auto frequency = static_cast<core::TrainingFrequency>(stmt.columnInt(14));

        core::UserConstraints c;
        c.constraintsMask = static_cast<uint16_t>(stmt.columnInt(15));
        if (!stmt.columnIsNull(16)) {
            c.customNotes = stmt.columnText(16);
        }
        c.availableEquipment = static_cast<core::Equipment>(stmt.columnInt(17));
        c.userAvoidAxialLoading = stmt.columnInt(18) != 0;
        c.userAvoidSpinalFlexion = stmt.columnInt(19) != 0;
        c.userAvoidOverheadMotion = stmt.columnInt(20) != 0;
        c.userAvoidDeepKneeFlexion = stmt.columnInt(21) != 0;
        c.userPreferUnilateral = stmt.columnInt(22) != 0;
        if (!stmt.columnIsNull(23)) {
            c.maxSessionMinutes = stmt.columnInt(23);
        }

        const auto createdAt = sql::fromIso(stmt.columnText(24));
        const auto updatedAt = sql::fromIso(stmt.columnText(25));

        return core::UserProfile::createFromDb(id,
                                               name,
                                               gender,
                                               m,
                                               level,
                                               goal,
                                               frequency,
                                               c,
                                               createdAt,
                                               updatedAt,
                                               estimated1RM);
    }

    // --- 1RM ---

    void saveEstimated1RMs(int64_t profileId, const std::map<core::ExerciseId, double>& e1rms) {
        {
            Statement del(db_.getRawConnection(),
                          "DELETE FROM user_estimated_1rm WHERE profile_id = ?;");
            del.bind(1, profileId);
            del.exec();
        }

        if (e1rms.empty()) {
            return;
        }

        Statement ins(db_.getRawConnection(),
                      "INSERT INTO user_estimated_1rm (profile_id, exercise_id, weight_kg) "
                      "VALUES (?, ?, ?);");
        for (const auto& [exerciseId, weightKg] : e1rms) {
            ins.bind(1, profileId);
            ins.bind(2, exerciseId);
            ins.bind(3, weightKg);
            ins.exec();
            ins.reset();
        }
    }

    std::map<core::ExerciseId, double> loadEstimated1RMs(int64_t profileId) {
        Statement stmt(db_.getRawConnection(),
                       "SELECT exercise_id, weight_kg FROM user_estimated_1rm "
                       "WHERE profile_id = ? ORDER BY exercise_id;");
        stmt.bind(1, profileId);

        std::map<core::ExerciseId, double> result;
        while (stmt.step()) {
            result[stmt.columnInt64(0)] = stmt.columnDouble(1);
        }
        return result;
    }

    // One query for every profile, so findAll() does not issue a query per row.
    std::map<int64_t, std::map<core::ExerciseId, double>> loadAllEstimated1RMs() {
        Statement stmt(db_.getRawConnection(),
                       "SELECT profile_id, exercise_id, weight_kg FROM user_estimated_1rm "
                       "ORDER BY profile_id, exercise_id;");

        std::map<int64_t, std::map<core::ExerciseId, double>> result;
        while (stmt.step()) {
            result[stmt.columnInt64(0)][stmt.columnInt64(1)] = stmt.columnDouble(2);
        }
        return result;
    }
};

} // namespace workout::infra