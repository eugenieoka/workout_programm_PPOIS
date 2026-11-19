# Console UI

The console UI is a Russian-language presentation layer on top of the
UI-agnostic `WorkoutService`. It does not contain SQL, domain validation, or
database access.

## Architecture

- `include/ui/ConsoleUI.h` — public `ConsoleUI` interface.
- `src/ui/ConsoleUI.cpp` — menu state machine, Russian text, input helpers, and
  formatting. Console-specific code stays in this single translation unit.
- `src/main.cpp` — parses `AppOptions`, initializes the database, and passes
  `std::cin` / `std::cout` to `ConsoleUI`.

`WorkoutService` never reads stdin, writes stdout, or formats Russian text. A
future iOS client can reuse the same service and map `ServiceError::code()` to
localized messages.

## Build and run

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure

./build/workout_cli
./build/workout_cli --help
```

Optional paths:

```bash
./build/workout_cli --db /tmp/workout.db --data-dir ./data
```

Environment variables `WORKOUT_DB` and `WORKOUT_DATA_DIR` are also supported.
The default database is `~/.workout/workout.db`; the schema and seed files are
loaded from the configured data directory on first run.

## Menus

### Main menu

1. Profiles
2. Exercise catalog
3. Generate a plan
4. My plans
0. Exit

### Profiles

Create, select, edit, and delete profiles. Editing covers every profile field:
name, age, gender, height, weight, body fat, training age, level, goal,
training frequency, available equipment, health constraints, and the session
limit.

Profile creation uses sensible defaults when the user presses Enter: age 30,
male, 175 cm, 75 kg, novice, hypertrophy, three days per week, and all
equipment. Blank body-fat input means unknown. Blank session-limit input means
no limit.

One-rep maxes are managed from the profiles menu: list current values, set a
value by exercise ID, or remove a value.

### Exercise catalog

Browse all exercises, search by name, or open an exercise card by ID. The card
shows description, muscles, equipment, difficulty, instructions, and video URL.

### Plan generation

The UI calls `WorkoutService::generatePlan`, shows the generated days and
exercises with resolved exercise names, and warns when any day is empty. The
user can save the plan with a name and optionally make it active.

### My plans

List, view, activate, and delete saved plans. Existing plans are intentionally
not editable in this MVP because the database fixes for targeted plan editing
(A3/A7) are deferred. The service still exposes `updatePlanExercise` for a
future UI.

## Error handling

- Service failures are caught as `ServiceError` and mapped to Russian messages.
- Validation failures from setters and service calls are reported without
  terminating the UI.
- EOF/closed input raises an internal `InputClosed`; `ConsoleUI::run` exits
  gracefully.

## Testing

`tests/ConsoleUITest.cpp` drives the UI with `std::istringstream` /
`std::ostringstream` against an in-memory database loaded from
`data/schema.sql` and `data/seed.sql`. It covers menu exit, EOF, invalid input,
profile creation and editing, one-rep max management, the exercise catalog,
plan generation/saving, and saved-plan operations.

Run the complete suite with:

```bash
ctest --test-dir build --output-on-failure
```
