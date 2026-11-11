#pragma once

#include "Database.h"
#include "core/Exercise.h"
#include "core/User.h"

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace workout::infra {

class ExerciseRepository {
public:
    explicit ExerciseRepository(Database& db) : db_(db) {}

    std::optional<core::Exercise> findById(core::ExerciseId id) {
        Statement stmt(db_.getRawConnection(), selectSql() + " WHERE id = ?;");
        stmt.bind(1, id);
        if (!stmt.step()) {
            return std::nullopt;
        }
        return mapRowToExercise(stmt, loadSecondaryMuscles(id));
    }

    std::optional<core::Exercise> findByName(const std::string& name) {
        Statement stmt(db_.getRawConnection(), selectSql() + " WHERE name = ?;");
        stmt.bind(1, name);
        if (!stmt.step()) {
            return std::nullopt;
        }
        return mapRowToExercise(stmt, loadSecondaryMuscles(stmt.columnInt64(0)));
    }

    std::vector<core::Exercise> findAll() {
        Statement stmt(db_.getRawConnection(), selectSql() + " ORDER BY id;");
        const auto secondary = loadAllSecondaryMuscles();

        std::vector<core::Exercise> result;
        while (stmt.step()) {
            const auto id = stmt.columnInt64(0);
            const auto it = secondary.find(id);
            result.push_back(mapRowToExercise(
                stmt, it == secondary.end() ? std::vector<core::MuscleGroup>{} : it->second));
        }
        return result;
    }

    // Exercises compatible with the user's constraints (for PlanGenerator).
    // Filtering happens in SQL so the full catalog is never materialized.
    std::vector<core::Exercise> findCompatible(const core::UserConstraints& constraints) {
        Statement stmt(db_.getRawConnection(), selectSql() + R"(
            WHERE (required_equipment & ?1) = required_equipment
              AND (?2 = 0 OR axial_loading = 0)
              AND (?3 = 0 OR spinal_flexion = 0)
              AND (?4 = 0 OR overhead_motion = 0)
              AND (?5 = 0 OR deep_knee_flexion = 0)
            ORDER BY id;
        )");

        stmt.bind(1, static_cast<int>(static_cast<uint16_t>(constraints.availableEquipment)));
        stmt.bind(2, constraints.shouldAvoidAxialLoading() ? 1 : 0);
        stmt.bind(3, constraints.shouldAvoidSpinalFlexion() ? 1 : 0);
        stmt.bind(4, constraints.shouldAvoidOverheadMotion() ? 1 : 0);
        stmt.bind(5, constraints.shouldAvoidDeepKneeFlexion() ? 1 : 0);

        const auto secondary = loadAllSecondaryMuscles();
        const std::vector<core::MuscleGroup> none;

        std::vector<core::Exercise> result;
        while (stmt.step()) {
            const auto id = stmt.columnInt64(0);
            const auto it = secondary.find(id);
            result.push_back(mapRowToExercise(stmt, it == secondary.end() ? none : it->second));
        }
        return result;
    }

    // INSERT or UPDATE by id. The id must be positive.
    core::ExerciseId save(const core::Exercise& exercise) {
        const core::ExerciseId id = exercise.getId();
        if (id <= 0) {
            throw std::invalid_argument("ExerciseRepository::save: exercise id must be positive");
        }

        db_.beginTransaction();
        try {
            Statement stmt(db_.getRawConnection(), R"(
                INSERT INTO exercises (
                    id, name, description, primary_muscle, structure, kind, purpose_type,
                    movement_pattern, required_equipment, optional_equipment, difficulty, met,
                    estimated_set_duration_sec, instructions, video_url, is_powerlifting_lift,
                    axial_loading, spinal_flexion, overhead_motion, deep_knee_flexion,
                    is_unilateral, regression_id, progression_id
                ) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
                ON CONFLICT(id) DO UPDATE SET
                    name = excluded.name,
                    description = excluded.description,
                    primary_muscle = excluded.primary_muscle,
                    structure = excluded.structure,
                    kind = excluded.kind,
                    purpose_type = excluded.purpose_type,
                    movement_pattern = excluded.movement_pattern,
                    required_equipment = excluded.required_equipment,
                    optional_equipment = excluded.optional_equipment,
                    difficulty = excluded.difficulty,
                    met = excluded.met,
                    estimated_set_duration_sec = excluded.estimated_set_duration_sec,
                    instructions = excluded.instructions,
                    video_url = excluded.video_url,
                    is_powerlifting_lift = excluded.is_powerlifting_lift,
                    axial_loading = excluded.axial_loading,
                    spinal_flexion = excluded.spinal_flexion,
                    overhead_motion = excluded.overhead_motion,
                    deep_knee_flexion = excluded.deep_knee_flexion,
                    is_unilateral = excluded.is_unilateral,
                    regression_id = excluded.regression_id,
                    progression_id = excluded.progression_id;
            )");

            bindColumns(stmt, exercise);
            stmt.exec();

            saveSecondaryMuscles(id, exercise.getSecondaryMuscles());

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

    // @return true if the exercise existed and was deleted.
    // @throw std::runtime_error if the exercise is used in a plan or a 1RM entry.
    bool remove(core::ExerciseId id) {
        Statement stmt(db_.getRawConnection(), "DELETE FROM exercises WHERE id = ?;");
        stmt.bind(1, id);
        stmt.exec();
        return db_.changes() > 0;
    }

private:
    Database& db_;

    static constexpr const char* kColumns =
        "id, name, description, primary_muscle, structure, kind, purpose_type, movement_pattern, "
        "required_equipment, optional_equipment, difficulty, met, estimated_set_duration_sec, "
        "instructions, video_url, is_powerlifting_lift, axial_loading, spinal_flexion, "
        "overhead_motion, deep_knee_flexion, is_unilateral, regression_id, progression_id";

    static std::string selectSql() {
        return std::string("SELECT ") + kColumns + " FROM exercises";
    }

    static void bindColumns(Statement& stmt, const core::Exercise& e) {
        stmt.bind(1, e.getId());
        stmt.bind(2, e.getName());
        stmt.bind(3, e.getDescription());
        stmt.bind(4, static_cast<int>(e.getPrimaryMuscle()));
        stmt.bind(5, static_cast<int>(e.getStructure()));
        stmt.bind(6, static_cast<int>(e.getKind()));
        stmt.bind(7, static_cast<int>(e.getPurposeType()));
        stmt.bind(8, static_cast<int>(e.getMovementPattern()));
        stmt.bind(9, static_cast<int>(static_cast<uint16_t>(e.getRequiredEquipment())));
        stmt.bind(10, static_cast<int>(static_cast<uint16_t>(e.getOptionalEquipment())));
        stmt.bind(11, static_cast<int>(e.getDifficulty()));
        stmt.bind(12, e.getMET());
        stmt.bind(13, e.getEstimatedSetDurationSec());
        stmt.bind(14, e.getInstructions());
        stmt.bind(15, e.getVideoUrl());
        stmt.bind(16, e.isPowerliftingLift() ? 1 : 0);
        stmt.bind(17, e.hasAxialLoading() ? 1 : 0);
        stmt.bind(18, e.hasSpinalFlexion() ? 1 : 0);
        stmt.bind(19, e.hasOverheadMotion() ? 1 : 0);
        stmt.bind(20, e.hasDeepKneeFlexion() ? 1 : 0);
        stmt.bind(21, e.isUnilateral() ? 1 : 0);

        if (e.getRegressionId()) {
            stmt.bind(22, *e.getRegressionId());
        } else {
            stmt.bindNull(22);
        }
        if (e.getProgressionId()) {
            stmt.bind(23, *e.getProgressionId());
        } else {
            stmt.bindNull(23);
        }
    }

    static core::Exercise mapRowToExercise(const Statement& stmt,
                                           const std::vector<core::MuscleGroup>& secondary) {
        core::ExerciseData data;
        data.id = stmt.columnInt64(0);
        data.name = stmt.columnText(1);
        data.description = stmt.columnText(2);
        data.primaryMuscle = static_cast<core::MuscleGroup>(stmt.columnInt(3));
        data.secondaryMuscles = secondary;
        data.structure = static_cast<core::ExerciseStructure>(stmt.columnInt(4));
        data.kind = static_cast<core::ExerciseKind>(stmt.columnInt(5));
        data.purposeType = static_cast<core::PurposeType>(stmt.columnInt(6));
        data.pattern = static_cast<core::MovementPattern>(stmt.columnInt(7));
        data.requiredEquipment = static_cast<core::Equipment>(
            static_cast<uint16_t>(stmt.columnInt(8)));
        data.optionalEquipment = static_cast<core::Equipment>(
            static_cast<uint16_t>(stmt.columnInt(9)));
        data.difficulty = static_cast<uint8_t>(stmt.columnInt(10));
        data.met = stmt.columnDouble(11);
        data.estimatedSetDurationSec = stmt.columnInt(12);
        data.instructions = stmt.columnText(13);
        data.videoUrl = stmt.columnText(14);
        data.isPowerliftingLift = stmt.columnInt(15) != 0;
        data.axialLoading = stmt.columnInt(16) != 0;
        data.spinalFlexion = stmt.columnInt(17) != 0;
        data.overheadMotion = stmt.columnInt(18) != 0;
        data.deepKneeFlexion = stmt.columnInt(19) != 0;
        data.isUnilateral = stmt.columnInt(20) != 0;

        if (!stmt.columnIsNull(21)) {
            data.regressionId = stmt.columnInt64(21);
        }
        if (!stmt.columnIsNull(22)) {
            data.progressionId = stmt.columnInt64(22);
        }

        return core::Exercise::createFromDb(data);
    }

    std::vector<core::MuscleGroup> loadSecondaryMuscles(core::ExerciseId id) {
        Statement stmt(db_.getRawConnection(),
                       "SELECT muscle_group FROM exercise_secondary_muscles "
                       "WHERE exercise_id = ? ORDER BY muscle_group;");
        stmt.bind(1, id);

        std::vector<core::MuscleGroup> result;
        while (stmt.step()) {
            result.push_back(static_cast<core::MuscleGroup>(stmt.columnInt(0)));
        }
        return result;
    }

    std::map<core::ExerciseId, std::vector<core::MuscleGroup>> loadAllSecondaryMuscles() {
        Statement stmt(db_.getRawConnection(),
                       "SELECT exercise_id, muscle_group FROM exercise_secondary_muscles "
                       "ORDER BY exercise_id, muscle_group;");

        std::map<core::ExerciseId, std::vector<core::MuscleGroup>> result;
        while (stmt.step()) {
            result[stmt.columnInt64(0)].push_back(
                static_cast<core::MuscleGroup>(stmt.columnInt(1)));
        }
        return result;
    }

    void saveSecondaryMuscles(core::ExerciseId id,
                              const std::vector<core::MuscleGroup>& muscles) {
        {
            Statement del(db_.getRawConnection(),
                          "DELETE FROM exercise_secondary_muscles WHERE exercise_id = ?;");
            del.bind(1, id);
            del.exec();
        }

        if (muscles.empty()) {
            return;
        }

        Statement ins(db_.getRawConnection(),
                      "INSERT OR IGNORE INTO exercise_secondary_muscles "
                      "(exercise_id, muscle_group) VALUES (?, ?);");
        for (const auto muscle : muscles) {
            ins.bind(1, id);
            ins.bind(2, static_cast<int>(muscle));
            ins.exec();
            ins.reset();
        }
    }
};

} // namespace workout::infra