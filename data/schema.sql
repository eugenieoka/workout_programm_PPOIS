PRAGMA foreign_keys = ON;

-- 1. Справочник упражнений (для ExerciseRepository)
CREATE TABLE IF NOT EXISTS exercises (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE,
    required_equipment INTEGER NOT NULL DEFAULT 0
);

-- 2. Профили пользователей (для UserRepository)
CREATE TABLE IF NOT EXISTS user_profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    gender INTEGER NOT NULL DEFAULT 0,
    
    -- BodyMetrics
    height_cm REAL NOT NULL,
    weight_kg REAL NOT NULL,
    body_fat_pct REAL DEFAULT -1.0,
    age_years INTEGER NOT NULL,
    training_age_months INTEGER NOT NULL DEFAULT 0,
    
    bmi REAL DEFAULT 0.0,
    bmr_kcal REAL DEFAULT 0.0,
    tdee_kcal REAL DEFAULT 0.0,
    ffmi REAL DEFAULT 0.0,
    
    -- Enums
    experience_level INTEGER NOT NULL DEFAULT 0,
    training_goal INTEGER NOT NULL DEFAULT 0,
    training_frequency INTEGER NOT NULL DEFAULT 3,
    
    -- Constraints
    constraints_mask INTEGER NOT NULL DEFAULT 0,
    custom_notes TEXT,
    available_equipment INTEGER NOT NULL DEFAULT 0,
    avoid_axial_loading BOOLEAN NOT NULL DEFAULT 0,
    avoid_spinal_flexion BOOLEAN NOT NULL DEFAULT 0,
    avoid_overhead_motion BOOLEAN NOT NULL DEFAULT 0,
    avoid_deep_knee_flexion BOOLEAN NOT NULL DEFAULT 0,
    prefer_unilateral BOOLEAN NOT NULL DEFAULT 0,
    max_session_minutes INTEGER,
    
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

-- 3. Оценки 1RM (связь с профилем)
CREATE TABLE IF NOT EXISTS user_estimated_1rm (
    profile_id INTEGER NOT NULL,
    exercise_id INTEGER NOT NULL,
    weight_kg REAL NOT NULL,
    PRIMARY KEY (profile_id, exercise_id),
    FOREIGN KEY (profile_id) REFERENCES user_profiles(id) ON DELETE CASCADE
);

-- 4. Планы тренировок (для PlanRepository, базовая структура)
CREATE TABLE IF NOT EXISTS workout_plans (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name TEXT NOT NULL,
    created_at TEXT NOT NULL,
    FOREIGN KEY (user_id) REFERENCES user_profiles(id) ON DELETE CASCADE
);
