#pragma once

#include "Database.h"
#include "SqlTime.h"
#include "core/Types.h"
#include "core/User.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace workout::infra {

// Database DTO layer. Mapping to core::WorkoutPlan is done by whoever implements
// PlanGenerator, so the repository does not depend on the unmerged feature/plan-generator branch.

struct PlanExerciseRecord {
    int64_t id = -1;
    int64_t planDayId = -1;
    core::ExerciseId exerciseId = -1;
    int position = -1;              // < 0 -> position in the vector
    int sets = 0;
    int reps = 0;
    int restSec = 60;
    std::optional<double> weightKg;
    std::string notes;
};

struct PlanDayRecord {
    int64_t id = -1;
    int64_t planId = -1;
    int dayIndex = -1;              // < 0 -> position in the vector
    std::string name;
    std::vector<PlanExerciseRecord> exercises;
};

struct PlanRecord {
    int64_t id = -1;
    int64_t userId = -1;
    std::string name;
    core::TrainingGoal goal = core::TrainingGoal::Strength;
    int daysPerWeek = 0;            // 0 -> use days.size()
    bool isActive = false;
    std::string notes;
    std::string createdAt;
    std::string updatedAt;
    std::vector<PlanDayRecord> days;
};

class PlanRepository {
public:
    explicit PlanRepository(Database& db) : db_(db) {}

    /**
     * @brief INSERT (id < 0) or UPDATE a plan together with its days and exercises.
     *
     * Days and exercises are replaced as a whole; ordering comes from
     * position/dayIndex, or from the vector order when the value is negative.
     */
    int64_t save(const PlanRecord& plan) {
        if (plan.userId <= 0) {
            throw std::invalid_argument("PlanRepository::save: user id must be positive");
        }

        db_.beginTransaction();
        try {
            const std::string now = sql::nowIso();
            const std::string createdAt = plan.createdAt.empty() ? now : plan.createdAt;
            const std::string updatedAt = plan.updatedAt.empty() ? now : plan.updatedAt;

            int daysPerWeek = plan.daysPerWeek;
            if (daysPerWeek <= 0) {
                daysPerWeek = static_cast<int>(plan.days.size());
            }
            if (daysPerWeek <= 0) {
                daysPerWeek = 1;
            }

            int64_t planId = plan.id;

            if (planId <= 0) {
                Statement ins(db_.getRawConnection(), R"(
                    INSERT INTO workout_plans
                        (user_id, name, goal, days_per_week, is_active, notes, created_at, updated_at)
                    VALUES (?, ?, ?, ?, 0, ?, ?, ?);
                )");
                ins.bind(1, plan.userId);
                ins.bind(2, plan.name);
                ins.bind(3, static_cast<int>(plan.goal));
                ins.bind(4, daysPerWeek);
                ins.bind(5, plan.notes);
                ins.bind(6, createdAt);
                ins.bind(7, updatedAt);
                ins.exec();
                planId = db_.lastInsertRowId();
            } else {
                Statement upd(db_.getRawConnection(), R"(
                    UPDATE workout_plans SET
                        user_id = ?, name = ?, goal = ?, days_per_week = ?,
                        is_active = 0, notes = ?, updated_at = ?
                    WHERE id = ?;
                )");
                upd.bind(1, plan.userId);
                upd.bind(2, plan.name);
                upd.bind(3, static_cast<int>(plan.goal));
                upd.bind(4, daysPerWeek);
                upd.bind(5, plan.notes);
                upd.bind(6, updatedAt);
                upd.bind(7, planId);
                upd.exec();
                if (db_.changes() == 0) {
                    throw std::runtime_error("Workout plan with id " + std::to_string(planId) +
                                             " not found");
                }
            }

            replaceDays(planId, plan.days);

            if (plan.isActive) {
                activate(planId, plan.userId);
            }

            db_.commit();
            return planId;
        } catch (...) {
            try {
                db_.rollback();
            } catch (...) {
                // Preserve the original exception.
            }
            throw;
        }
    }

    std::optional<PlanRecord> findById(int64_t planId) {
        Statement stmt(db_.getRawConnection(), R"(
            SELECT id, user_id, name, goal, days_per_week, is_active, notes, created_at, updated_at
            FROM workout_plans WHERE id = ?;
        )");
        stmt.bind(1, planId);
        if (!stmt.step()) {
            return std::nullopt;
        }

        PlanRecord plan = mapPlanRow(stmt);
        plan.days = loadDays(planId);
        return plan;
    }

    std::vector<PlanRecord> findByUser(int64_t userId) {
        Statement stmt(db_.getRawConnection(), R"(
            SELECT id, user_id, name, goal, days_per_week, is_active, notes, created_at, updated_at
            FROM workout_plans WHERE user_id = ? ORDER BY id;
        )");
        stmt.bind(1, userId);

        std::vector<PlanRecord> result;
        while (stmt.step()) {
            PlanRecord plan = mapPlanRow(stmt);
            plan.days = loadDays(plan.id);
            result.push_back(std::move(plan));
        }
        return result;
    }

    // Make the plan active, clearing the flag on the user's other plans.
    bool setActive(int64_t planId) {
        int64_t userId = -1;
        {
            Statement get(db_.getRawConnection(),
                          "SELECT user_id FROM workout_plans WHERE id = ?;");
            get.bind(1, planId);
            if (!get.step()) {
                return false;
            }
            userId = get.columnInt64(0);
        }

        db_.beginTransaction();
        try {
            {
                Statement deactivate(db_.getRawConnection(),
                                     "UPDATE workout_plans SET is_active = 0 WHERE user_id = ?;");
                deactivate.bind(1, userId);
                deactivate.exec();
            }
            {
                Statement activate(db_.getRawConnection(),
                                   "UPDATE workout_plans SET is_active = 1 WHERE id = ?;");
                activate.bind(1, planId);
                activate.exec();
            }
            db_.commit();
            return true;
        } catch (...) {
            try {
                db_.rollback();
            } catch (...) {
            }
            throw;
        }
    }

    bool remove(int64_t planId) {
        Statement stmt(db_.getRawConnection(), "DELETE FROM workout_plans WHERE id = ?;");
        stmt.bind(1, planId);
        stmt.exec();
        return db_.changes() > 0;
    }

private:
    Database& db_;

    static PlanRecord mapPlanRow(const Statement& stmt) {
        PlanRecord plan;
        plan.id = stmt.columnInt64(0);
        plan.userId = stmt.columnInt64(1);
        plan.name = stmt.columnText(2);
        plan.goal = static_cast<core::TrainingGoal>(stmt.columnInt(3));
        plan.daysPerWeek = stmt.columnInt(4);
        plan.isActive = stmt.columnInt(5) != 0;
        plan.notes = stmt.columnText(6);
        plan.createdAt = stmt.columnText(7);
        plan.updatedAt = stmt.columnText(8);
        return plan;
    }

    void replaceDays(int64_t planId, const std::vector<PlanDayRecord>& days) {
        {
            Statement del(db_.getRawConnection(),
                          "DELETE FROM plan_days WHERE plan_id = ?;");
            del.bind(1, planId);
            del.exec();
        }

        if (days.empty()) {
            return;
        }

        Statement dayIns(db_.getRawConnection(),
                         "INSERT INTO plan_days (plan_id, day_index, name) VALUES (?, ?, ?);");
        Statement exerciseIns(db_.getRawConnection(), R"(
            INSERT INTO plan_exercises
                (plan_day_id, exercise_id, position, sets, reps, rest_sec, weight_kg, notes)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?);
        )");

        int autoDayIndex = 0;
        for (const auto& day : days) {
            const int dayIndex = day.dayIndex >= 0 ? day.dayIndex : autoDayIndex;
            dayIns.bind(1, planId);
            dayIns.bind(2, dayIndex);
            dayIns.bind(3, day.name);
            dayIns.exec();
            const int64_t dayId = db_.lastInsertRowId();
            dayIns.reset();

            int autoPosition = 0;
            for (const auto& exercise : day.exercises) {
                if (exercise.exerciseId <= 0) {
                    throw std::invalid_argument(
                        "PlanRepository::save: exercise id must be positive");
                }
                if (exercise.sets <= 0 || exercise.reps <= 0) {
                    throw std::invalid_argument(
                        "PlanRepository::save: sets and reps must be positive");
                }

                const int position =
                    exercise.position >= 0 ? exercise.position : autoPosition;

                exerciseIns.bind(1, dayId);
                exerciseIns.bind(2, exercise.exerciseId);
                exerciseIns.bind(3, position);
                exerciseIns.bind(4, exercise.sets);
                exerciseIns.bind(5, exercise.reps);
                exerciseIns.bind(6, exercise.restSec);
                if (exercise.weightKg) {
                    exerciseIns.bind(7, *exercise.weightKg);
                } else {
                    exerciseIns.bindNull(7);
                }
                exerciseIns.bind(8, exercise.notes);
                exerciseIns.exec();
                exerciseIns.reset();

                ++autoPosition;
            }
            ++autoDayIndex;
        }
    }

    void activate(int64_t planId, int64_t userId) {
        {
            Statement deactivate(db_.getRawConnection(),
                                 "UPDATE workout_plans SET is_active = 0 "
                                 "WHERE user_id = ? AND id <> ?;");
            deactivate.bind(1, userId);
            deactivate.bind(2, planId);
            deactivate.exec();
        }
        Statement activateStmt(db_.getRawConnection(),
                               "UPDATE workout_plans SET is_active = 1 WHERE id = ?;");
        activateStmt.bind(1, planId);
        activateStmt.exec();
    }

    std::vector<PlanDayRecord> loadDays(int64_t planId) {
        Statement stmt(db_.getRawConnection(),
                       "SELECT id, plan_id, day_index, name FROM plan_days "
                       "WHERE plan_id = ? ORDER BY day_index;");
        stmt.bind(1, planId);

        std::vector<PlanDayRecord> days;
        while (stmt.step()) {
            PlanDayRecord day;
            day.id = stmt.columnInt64(0);
            day.planId = stmt.columnInt64(1);
            day.dayIndex = stmt.columnInt(2);
            day.name = stmt.columnText(3);
            day.exercises = loadExercises(day.id);
            days.push_back(std::move(day));
        }
        return days;
    }

    std::vector<PlanExerciseRecord> loadExercises(int64_t planDayId) {
        Statement stmt(db_.getRawConnection(), R"(
            SELECT id, plan_day_id, exercise_id, position, sets, reps, rest_sec, weight_kg, notes
            FROM plan_exercises WHERE plan_day_id = ? ORDER BY position;
        )");
        stmt.bind(1, planDayId);

        std::vector<PlanExerciseRecord> result;
        while (stmt.step()) {
            PlanExerciseRecord exercise;
            exercise.id = stmt.columnInt64(0);
            exercise.planDayId = stmt.columnInt64(1);
            exercise.exerciseId = stmt.columnInt64(2);
            exercise.position = stmt.columnInt(3);
            exercise.sets = stmt.columnInt(4);
            exercise.reps = stmt.columnInt(5);
            exercise.restSec = stmt.columnInt(6);
            if (!stmt.columnIsNull(7)) {
                exercise.weightKg = stmt.columnDouble(7);
            }
            exercise.notes = stmt.columnText(8);
            result.push_back(std::move(exercise));
        }
        return result;
    }
};

} // namespace workout::infra