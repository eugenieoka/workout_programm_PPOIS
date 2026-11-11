PRAGMA foreign_keys = ON;

-- Schema version for future migrations.
PRAGMA user_version = 1;

-- 1. Exercise catalog (for ExerciseRepository)
CREATE TABLE IF NOT EXISTS exercises (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    description TEXT NOT NULL DEFAULT '',

    -- MuscleGroup (see include/core/Exercise.h)
    primary_muscle INTEGER NOT NULL DEFAULT 0,

    -- ExerciseStructure / ExerciseKind / PurposeType / MovementPattern
    structure INTEGER NOT NULL DEFAULT 0,
    kind INTEGER NOT NULL DEFAULT 0,
    purpose_type INTEGER NOT NULL DEFAULT 0,
    movement_pattern INTEGER NOT NULL DEFAULT 9,

    -- Equipment bitmask (see include/core/Types.h)
    required_equipment INTEGER NOT NULL DEFAULT 0,
    optional_equipment INTEGER NOT NULL DEFAULT 0,

    difficulty INTEGER NOT NULL DEFAULT 3 CHECK (difficulty BETWEEN 1 AND 5),
    met REAL NOT NULL DEFAULT 0.0 CHECK (met >= 0.0),
    estimated_set_duration_sec INTEGER NOT NULL DEFAULT 60
        CHECK (estimated_set_duration_sec > 0),

    instructions TEXT NOT NULL DEFAULT '',
    video_url TEXT NOT NULL DEFAULT '',

    is_powerlifting_lift INTEGER NOT NULL DEFAULT 0 CHECK (is_powerlifting_lift IN (0, 1)),

    -- Health constraint flags (see UserConstraints)
    axial_loading INTEGER NOT NULL DEFAULT 0 CHECK (axial_loading IN (0, 1)),
    spinal_flexion INTEGER NOT NULL DEFAULT 0 CHECK (spinal_flexion IN (0, 1)),
    overhead_motion INTEGER NOT NULL DEFAULT 0 CHECK (overhead_motion IN (0, 1)),
    deep_knee_flexion INTEGER NOT NULL DEFAULT 0 CHECK (deep_knee_flexion IN (0, 1)),
    is_unilateral INTEGER NOT NULL DEFAULT 0 CHECK (is_unilateral IN (0, 1)),

    -- Substitution chain
    regression_id INTEGER REFERENCES exercises(id) ON DELETE SET NULL,
    progression_id INTEGER REFERENCES exercises(id) ON DELETE SET NULL
);

-- Secondary muscle groups (ExerciseData::secondaryMuscles)
CREATE TABLE IF NOT EXISTS exercise_secondary_muscles (
    exercise_id INTEGER NOT NULL REFERENCES exercises(id) ON DELETE CASCADE,
    muscle_group INTEGER NOT NULL,
    PRIMARY KEY (exercise_id, muscle_group)
);

-- 2. User profiles (for UserRepository)
CREATE TABLE IF NOT EXISTS user_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    gender INTEGER NOT NULL DEFAULT 0 CHECK (gender IN (0, 1, 2)),

    -- BodyMetrics
    height_cm REAL NOT NULL CHECK (height_cm > 0.0),
    weight_kg REAL NOT NULL CHECK (weight_kg > 0.0),
    body_fat_pct REAL DEFAULT -1.0
        CHECK (body_fat_pct = -1.0 OR (body_fat_pct >= 3.0 AND body_fat_pct <= 50.0)),
    age_years INTEGER NOT NULL CHECK (age_years >= 0),
    training_age_months INTEGER NOT NULL DEFAULT 0 CHECK (training_age_months >= 0),

    bmi REAL DEFAULT 0.0,
    bmr_kcal REAL DEFAULT 0.0,
    tdee_kcal REAL DEFAULT 0.0,
    ffmi REAL DEFAULT 0.0,

    -- Enums
    experience_level INTEGER NOT NULL DEFAULT 0 CHECK (experience_level IN (0, 1, 2)),
    training_goal INTEGER NOT NULL DEFAULT 0 CHECK (training_goal IN (0, 1, 2, 3)),
    training_frequency INTEGER NOT NULL DEFAULT 3 CHECK (training_frequency BETWEEN 2 AND 6),

    -- Constraints
    constraints_mask INTEGER NOT NULL DEFAULT 0,
    custom_notes TEXT,
    available_equipment INTEGER NOT NULL DEFAULT 0,
    avoid_axial_loading INTEGER NOT NULL DEFAULT 0 CHECK (avoid_axial_loading IN (0, 1)),
    avoid_spinal_flexion INTEGER NOT NULL DEFAULT 0 CHECK (avoid_spinal_flexion IN (0, 1)),
    avoid_overhead_motion INTEGER NOT NULL DEFAULT 0 CHECK (avoid_overhead_motion IN (0, 1)),
    avoid_deep_knee_flexion INTEGER NOT NULL DEFAULT 0 CHECK (avoid_deep_knee_flexion IN (0, 1)),
    prefer_unilateral INTEGER NOT NULL DEFAULT 0 CHECK (prefer_unilateral IN (0, 1)),
    max_session_minutes INTEGER CHECK (max_session_minutes IS NULL OR max_session_minutes > 0),

    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
    updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now'))
);

CREATE INDEX IF NOT EXISTS idx_user_profiles_name ON user_profiles(name);

-- 3. 1RM estimates (linked to a profile and an exercise)
CREATE TABLE IF NOT EXISTS user_estimated_1rm (
    profile_id INTEGER NOT NULL,
    exercise_id INTEGER NOT NULL,
    weight_kg REAL NOT NULL CHECK (weight_kg > 0.0),
    PRIMARY KEY (profile_id, exercise_id),
    FOREIGN KEY (profile_id) REFERENCES user_profiles(id) ON DELETE CASCADE,
    FOREIGN KEY (exercise_id) REFERENCES exercises(id) ON DELETE RESTRICT
);

-- 4. Workout plans
CREATE TABLE IF NOT EXISTS workout_plans (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name TEXT NOT NULL DEFAULT '',
    goal INTEGER NOT NULL DEFAULT 0 CHECK (goal IN (0, 1, 2, 3)),
    days_per_week INTEGER NOT NULL DEFAULT 3 CHECK (days_per_week BETWEEN 1 AND 7),
    is_active INTEGER NOT NULL DEFAULT 0 CHECK (is_active IN (0, 1)),
    notes TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
    updated_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ', 'now')),
    FOREIGN KEY (user_id) REFERENCES user_profiles(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_workout_plans_user ON workout_plans(user_id);

-- A user cannot have more than one active plan.
CREATE UNIQUE INDEX IF NOT EXISTS idx_workout_plans_one_active
    ON workout_plans(user_id) WHERE is_active = 1;

-- 5. Plan days
CREATE TABLE IF NOT EXISTS plan_days (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    plan_id INTEGER NOT NULL,
    day_index INTEGER NOT NULL CHECK (day_index >= 0),
    name TEXT NOT NULL DEFAULT '',
    UNIQUE (plan_id, day_index),
    FOREIGN KEY (plan_id) REFERENCES workout_plans(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_plan_days_plan ON plan_days(plan_id);

-- 6. Exercises within a plan day
CREATE TABLE IF NOT EXISTS plan_exercises (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    plan_day_id INTEGER NOT NULL,
    exercise_id INTEGER NOT NULL,
    position INTEGER NOT NULL CHECK (position >= 0),
    sets INTEGER NOT NULL CHECK (sets > 0),
    reps INTEGER NOT NULL CHECK (reps > 0),
    rest_sec INTEGER NOT NULL DEFAULT 60 CHECK (rest_sec >= 0),
    weight_kg REAL CHECK (weight_kg IS NULL OR weight_kg >= 0.0),
    notes TEXT NOT NULL DEFAULT '',
    UNIQUE (plan_day_id, position),
    FOREIGN KEY (plan_day_id) REFERENCES plan_days(id) ON DELETE CASCADE,
    FOREIGN KEY (exercise_id) REFERENCES exercises(id) ON DELETE RESTRICT
);

CREATE INDEX IF NOT EXISTS idx_plan_exercises_day ON plan_exercises(plan_day_id);
CREATE INDEX IF NOT EXISTS idx_plan_exercises_exercise ON plan_exercises(exercise_id);