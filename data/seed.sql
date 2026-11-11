-- Базовый каталог упражнений.
-- Числовые значения соответствуют enum'ам из include/core/Exercise.h и Types.h.
INSERT OR IGNORE INTO exercises (
    id, name, primary_muscle, structure, kind, purpose_type, movement_pattern,
    required_equipment, optional_equipment, difficulty, met, estimated_set_duration_sec,
    is_powerlifting_lift, axial_loading, spinal_flexion, overhead_motion,
    deep_knee_flexion, is_unilateral, regression_id, progression_id
) VALUES
-- id, название, primary, struct, kind, purpose, pattern, reqEq, optEq, diff, met, setSec, PL, axial, spinal, overhead, knee, uni, reg, prog
( 1, 'Barbell Back Squat',      5, 0, 0, 0, 0,    5,    0, 4, 6.0,  90, 1, 1, 0, 0, 1, 0, NULL, NULL),
( 2, 'Overhead Press',         9, 0, 0, 0, 4,    1,    0, 4, 5.0,  75, 1, 0, 0, 1, 0, 0, NULL, NULL),
( 3, 'Bench Press',            0, 0, 0, 0, 3, 1025,    0, 3, 5.0,  75, 1, 0, 0, 0, 0, 0, NULL, NULL),
( 4, 'Pull-up',                2, 0, 0, 0, 6,   32,    0, 4, 5.0,  60, 0, 0, 0, 0, 0, 0, NULL, NULL),
( 5, 'Deadlift',               7, 0, 0, 0, 1,    1,    0, 5, 6.0,  90, 1, 1, 0, 0, 0, 0, NULL, NULL),
( 6, 'Romanian Deadlift',      7, 0, 0, 1, 1,    1,    0, 3, 5.0,  75, 0, 1, 0, 0, 0, 0, NULL, 5),
( 7, 'Front Squat',            5, 0, 0, 1, 0,    5,    0, 4, 6.0,  90, 0, 1, 0, 0, 1, 0, 1, NULL),
( 8, 'Bulgarian Split Squat',  5, 0, 0, 1, 2, 1026,    0, 3, 5.0,  60, 0, 0, 0, 0, 1, 1, NULL, NULL),
( 9, 'Walking Lunge',          5, 0, 0, 1, 2,    2,    0, 2, 4.0,  60, 0, 0, 0, 0, 1, 1, NULL, NULL),
(10, 'Leg Press',              5, 0, 0, 1, 0,   16,    0, 2, 4.0,  60, 0, 0, 0, 0, 1, 0, NULL, NULL),
(11, 'Leg Curl',               7, 1, 0, 1, 9,   16,    0, 2, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(12, 'Leg Extension',          5, 1, 0, 1, 9,   16,    0, 2, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(13, 'Standing Calf Raise',    8, 1, 0, 1, 9,   16,    0, 1, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(14, 'Barbell Row',            2, 0, 0, 0, 5,    1,    0, 3, 5.0,  75, 0, 1, 0, 0, 0, 0, NULL, NULL),
(15, 'Dumbbell Row',           2, 0, 0, 1, 5, 1026,    0, 2, 4.0,  60, 0, 0, 0, 0, 0, 1, NULL, 14),
(16, 'Lat Pulldown',           2, 0, 0, 1, 6,    8,    0, 2, 4.0,  60, 0, 0, 0, 0, 0, 0, 4, NULL),
(17, 'Seated Cable Row',       2, 0, 0, 1, 5,    8,    0, 2, 4.0,  60, 0, 0, 0, 0, 0, 0, NULL, NULL),
(18, 'Dips',                   1, 0, 0, 1, 4,   64,    0, 3, 5.0,  60, 0, 0, 0, 0, 0, 0, NULL, NULL),
(19, 'Push-up',                1, 0, 0, 1, 3,    0,    0, 1, 4.0,  45, 0, 0, 0, 0, 0, 0, NULL, 3),
(20, 'Plank',                 15, 1, 0, 2, 9,    0,    0, 1, 0.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(21, 'Hanging Leg Raise',     15, 0, 0, 1, 9,   32,    0, 3, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(22, 'Face Pull',             11, 1, 0, 2, 5,    8,    0, 1, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(23, 'Lateral Raise',         10, 1, 0, 1, 9,    2,    0, 1, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(24, 'Dumbbell Bicep Curl',   12, 1, 0, 1, 9,    2,    0, 1, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(25, 'Triceps Pushdown',      13, 1, 0, 1, 9,    8,    0, 1, 3.0,  45, 0, 0, 0, 0, 0, 0, NULL, NULL),
(26, 'Treadmill Run',          5, 0, 2, 3, 9,  512,    0, 2, 9.0,  60, 0, 0, 0, 0, 0, 0, NULL, NULL),
(27, 'Rowing Machine',         2, 0, 2, 3, 5,  512,    0, 2, 8.0,  60, 0, 0, 0, 0, 0, 0, NULL, NULL);

-- Вторичные мышечные группы.
INSERT OR IGNORE INTO exercise_secondary_muscles (exercise_id, muscle_group) VALUES
( 1,  6), ( 1,  7), ( 1, 17),
( 2, 13), ( 2, 10), ( 2, 15),
( 3, 13), ( 3,  9),
( 4, 12), ( 4,  4),
( 5,  6), ( 5,  2), ( 5,  3), ( 5, 17), ( 5, 14),
( 6,  6), ( 6, 17),
( 7,  6), ( 7, 15), ( 7, 17),
( 8,  6), ( 8,  7),
( 9,  6),
(10,  6),
(11,  8),
(14,  4), (14, 12), (14, 17),
(15,  4), (15, 12),
(16, 12), (16,  4),
(17,  4), (17, 12),
(18, 13), (18,  9),
(19, 13), (19,  9), (19, 15),
(20, 16), (20, 17),
(21, 16),
(22,  3), (22,  4),
(24, 14),
(26,  6), (26,  8),
(27, 12), (27,  4);

-- Цепочки замен, требующие ссылок вперёд, проставляем после вставки всех строк.
UPDATE exercises SET progression_id = 7 WHERE id = 1;