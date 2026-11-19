#include "ui/ConsoleUI.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/ExerciseFilter.h"
#include "app/PlanInfo.h"
#include "core/Exercise.h"
#include "core/User.h"
#include "core/WorkoutPlan.h"

namespace workout::ui {
namespace {

class InputClosed : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

constexpr std::string_view appTitle = "Workout Planner";
constexpr std::string_view goodbye = "До свидания.";

constexpr std::string_view mainMenuText = R"(
=== Главное меню ===
1. Профили
2. Каталог упражнений
3. Сгенерировать план
4. Мои планы
0. Выход
)";

constexpr std::string_view profileMenuText = R"(
=== Профили ===
1. Список профилей
2. Создать профиль
3. Выбрать профиль
4. Редактировать профиль
5. Управление 1ПМ
6. Удалить профиль
0. Назад
)";

constexpr std::string_view editProfileMenu = R"(
=== Редактирование профиля ===
1. Имя
2. Возраст
3. Пол
4. Рост
5. Вес
6. Процент жира
7. Тренировочный стаж
8. Уровень
9. Цель
10. Частота тренировок
11. Оборудование
12. Ограничения здоровья
13. Лимит сессии
0. Назад
)";

constexpr std::string_view catalogMenuText = R"(
=== Каталог упражнений ===
1. Все упражнения
2. Поиск по названию
3. Карточка упражнения
0. Назад
)";

constexpr std::string_view plansMenuText = R"(
=== Мои планы ===
1. Список планов
2. Просмотр плана
3. Активировать план
4. Удалить план
0. Назад
)";

constexpr std::string_view oneRepMaxMenu = R"(
=== 1ПМ ===
1. Задать 1ПМ
2. Удалить 1ПМ
0. Назад
)";

constexpr std::string_view newProfileTitle = "\n=== Новый профиль ===\n";

constexpr std::string_view choicePrompt = "Выбор: ";
constexpr std::string_view promptName = "Имя: ";
constexpr std::string_view promptAge = "Возраст [13-100] (Enter — 30): ";
constexpr std::string_view promptGender =
    "Пол (1 — мужской, 2 — женский, 3 — другой) [Enter — 1]: ";
constexpr std::string_view promptHeight = "Рост, см (Enter — 175): ";
constexpr std::string_view promptWeight = "Вес, кг (Enter — 75): ";
constexpr std::string_view promptBodyFat =
    "Процент жира (Enter — неизвестно): ";
constexpr std::string_view promptTrainingAge =
    "Тренировочный стаж, мес (Enter — 0): ";
constexpr std::string_view promptLevel =
    "Уровень (1 — новичок, 2 — средний, 3 — продвинутый) [Enter — 1]: ";
constexpr std::string_view promptGoal =
    "Цель (1 — сила, 2 — гипертрофия, 3 — похудение, 4 — поддержание) [Enter — 2]: ";
constexpr std::string_view promptFrequency =
    "Тренировок в неделю (2-6) [Enter — 3]: ";
constexpr std::string_view promptEquipment =
    "Оборудование (номера через запятую, 0 — без оборудования) [Enter — оставить текущее]: ";
constexpr std::string_view promptHealth =
    "Ограничения здоровья (номера через запятую, 0 — нет) [Enter — оставить текущее]: ";
constexpr std::string_view promptMaxSession =
    "Лимит сессии, мин (10-300, Enter — без лимита): ";
constexpr std::string_view promptFilterName =
    "Подстрока названия (Enter — все): ";
constexpr std::string_view promptProfileId = "ID профиля: ";
constexpr std::string_view promptPlanId = "ID плана: ";
constexpr std::string_view promptExerciseId = "ID упражнения: ";
constexpr std::string_view promptPlanName = "Название плана: ";
constexpr std::string_view promptOneRepMaxKg = "1ПМ, кг: ";
constexpr std::string_view savePlanPrompt = "Сохранить план? (д/н): ";
constexpr std::string_view makeActivePrompt = "Сделать план активным? (д/н): ";
constexpr std::string_view confirmDeleteProfile = "Удалить профиль? (д/н): ";
constexpr std::string_view confirmDeletePlan = "Удалить план? (д/н): ";

constexpr std::string_view emptyValue = "Значение не может быть пустым.";
constexpr std::string_view invalidNumber = "Некорректное число. Попробуйте снова.";
constexpr std::string_view numberOutOfRange = "Число вне допустимого диапазона.";
constexpr std::string_view yesNoHint = "Введите д или н.";
constexpr std::string_view inputClosed = "Ввод завершён.";
constexpr std::string_view noSelectedProfile = "Сначала выберите профиль.";
constexpr std::string_view noProfiles = "Профилей пока нет.";
constexpr std::string_view noPlans = "Планов пока нет.";
constexpr std::string_view noOneRepMaxes = "1ПМ пока не заданы.";
constexpr std::string_view nothingFound = "Ничего не найдено.";
constexpr std::string_view emptyPlanGenerated =
    "Не удалось подобрать упражнения для плана.";
constexpr std::string_view emptyDaysWarning =
    "Внимание: некоторые дни получились пустыми.";
constexpr std::string_view profileInvalid =
    "Не удалось создать профиль. Проверьте введённые данные.";
constexpr std::string_view profileCreated = "Профиль создан. ID: ";
constexpr std::string_view profileSaved = "Профиль сохранён.";
constexpr std::string_view profileSelected = "Выбран профиль: ";
constexpr std::string_view oneRepMaxSaved = "1ПМ сохранён.";
constexpr std::string_view oneRepMaxRemoved = "1ПМ удалён.";
constexpr std::string_view profileDeleted = "Профиль удалён.";
constexpr std::string_view planSaved = "План сохранён. ID: ";
constexpr std::string_view planActivated = "План активирован.";
constexpr std::string_view planDeleted = "План удалён.";
constexpr std::string_view notFound = "Не найдено.";
constexpr std::string_view unknownCommand = "Неизвестная команда.";
constexpr std::string_view errorPrefix = "Ошибка: ";
constexpr std::string_view selectedMarker = " *";
constexpr std::string_view currentProfilePrefix = "Текущий профиль: ";
constexpr std::string_view serviceProfileNotFound = "Профиль не найден.";
constexpr std::string_view serviceExerciseNotFound = "Упражнение не найдено.";
constexpr std::string_view serviceEmptyPlan = "План пуст.";
constexpr std::string_view serviceInvalidArgument = "Некорректные данные.";
constexpr std::string_view serviceUnknown = "Ошибка приложения.";

constexpr std::uint16_t kAllEquipmentMask = 0x07FFu;

[[nodiscard]] core::Equipment allEquipment() {
    return static_cast<core::Equipment>(kAllEquipmentMask);
}

[[nodiscard]] std::string formatDouble(double value, int precision = 1) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

[[nodiscard]] std::string trimCopy(std::string_view text) {
    const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };

    const auto begin = std::find_if(text.begin(), text.end(), notSpace);
    if (begin == text.end()) {
        return {};
    }
    const auto end = std::find_if(text.rbegin(), text.rend(), notSpace).base();
    return std::string(begin, end);
}

[[nodiscard]] std::string joinOrNone(const std::vector<std::string>& values,
                                     const std::string& noneText) {
    if (values.empty()) {
        return noneText;
    }

    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << values[i];
    }
    return out.str();
}

[[nodiscard]] std::string formatGender(core::Gender gender) {
    switch (gender) {
        case core::Gender::Male: return "мужской";
        case core::Gender::Female: return "женский";
        case core::Gender::Other: return "другой";
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatLevel(core::ExperienceLevel level) {
    switch (level) {
        case core::ExperienceLevel::Novice: return "новичок";
        case core::ExperienceLevel::Intermediate: return "средний";
        case core::ExperienceLevel::Advanced: return "продвинутый";
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatGoal(core::TrainingGoal goal) {
    switch (goal) {
        case core::TrainingGoal::Strength: return "сила";
        case core::TrainingGoal::Hypertrophy: return "гипертрофия";
        case core::TrainingGoal::WeightLoss: return "похудение";
        case core::TrainingGoal::Maintenance: return "поддержание";
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatFrequency(core::TrainingFrequency frequency) {
    const int days = static_cast<int>(frequency);
    const std::string suffix = days >= 2 && days <= 4 ? " раза в неделю"
                                                     : " раз в неделю";
    return std::to_string(days) + suffix;
}

[[nodiscard]] std::string formatHealthConstraint(core::HealthConstraint constraint) {
    switch (constraint) {
        case core::HealthConstraint::None: return "нет";
        case core::HealthConstraint::LowerBackPain: return "нижняя часть спины";
        case core::HealthConstraint::KneePain: return "колени";
        case core::HealthConstraint::ShoulderImpingement: return "плечи";
        case core::HealthConstraint::ElbowTendinopathy: return "локти";
        case core::HealthConstraint::WristPain: return "запястья";
        case core::HealthConstraint::HipImpingement: return "тазобедренные суставы";
        case core::HealthConstraint::CardiovascularIssue: return "сердечно-сосудистые";
        case core::HealthConstraint::Hernia: return "грыжа";
        case core::HealthConstraint::Custom: return "другое";
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatMuscleGroup(core::MuscleGroup muscle) {
    switch (muscle) {
        case core::MuscleGroup::Chest_Upper: return "верх груди";
        case core::MuscleGroup::Chest_Lower: return "низ груди";
        case core::MuscleGroup::Back_Lats: return "широчайшие";
        case core::MuscleGroup::Back_Traps: return "трапеции";
        case core::MuscleGroup::Back_Rhomboids: return "ромбовидные";
        case core::MuscleGroup::Legs_Quads: return "квадрицепсы";
        case core::MuscleGroup::Legs_Glutes: return "ягодицы";
        case core::MuscleGroup::Legs_Hamstrings: return "бицепс бедра";
        case core::MuscleGroup::Legs_Calves: return "икры";
        case core::MuscleGroup::Shoulders_Front: return "передние дельты";
        case core::MuscleGroup::Shoulders_Side: return "средние дельты";
        case core::MuscleGroup::Shoulders_Rear: return "задние дельты";
        case core::MuscleGroup::Arms_Biceps: return "бицепс";
        case core::MuscleGroup::Arms_Triceps: return "трицепс";
        case core::MuscleGroup::Arms_Forearms: return "предплечья";
        case core::MuscleGroup::Core_Abs: return "пресс";
        case core::MuscleGroup::Core_Obliques: return "косые мышцы";
        case core::MuscleGroup::Core_LowerBack: return "поясница";
        case core::MuscleGroup::Count: break;
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatEquipment(core::Equipment equipment) {
    switch (equipment) {
        case core::Equipment::None: return "без оборудования";
        case core::Equipment::Barbell: return "штанга";
        case core::Equipment::Dumbbells: return "гантели";
        case core::Equipment::PowerRack: return "силовая рама";
        case core::Equipment::CableMachine: return "блочный тренажёр";
        case core::Equipment::Machines: return "тренажёры";
        case core::Equipment::PullUpBar: return "турник";
        case core::Equipment::DipStation: return "брусья";
        case core::Equipment::Bands: return "резинки";
        case core::Equipment::Kettlebells: return "гири";
        case core::Equipment::CardioMachines: return "кардиотренажёры";
        case core::Equipment::Bench: return "скамья";
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatEquipmentMask(core::Equipment mask) {
    if (mask == core::Equipment::None) {
        return formatEquipment(core::Equipment::None);
    }

    std::vector<std::string> values;
    for (int bit = 0; bit < 11; ++bit) {
        const auto equipment = static_cast<core::Equipment>(1u << bit);
        if (core::hasEquipment(mask, equipment)) {
            values.push_back(formatEquipment(equipment));
        }
    }
    return joinOrNone(values, formatEquipment(core::Equipment::None));
}

[[nodiscard]] std::string formatHealthConstraints(
    const core::UserConstraints& constraints) {
    std::vector<std::string> values;
    for (int value = 1; value <= 9; ++value) {
        const auto constraint = static_cast<core::HealthConstraint>(value);
        if (constraints.hasConstraint(constraint)) {
            values.push_back(formatHealthConstraint(constraint));
        }
    }

    if (constraints.userAvoidAxialLoading) {
        values.push_back("избегать осевой нагрузки");
    }
    if (constraints.userAvoidSpinalFlexion) {
        values.push_back("избегать сгибания позвоночника");
    }
    if (constraints.userAvoidOverheadMotion) {
        values.push_back("избегать движений над головой");
    }
    if (constraints.userAvoidDeepKneeFlexion) {
        values.push_back("избегать глубокого сгибания коленей");
    }
    if (constraints.userPreferUnilateral) {
        values.push_back("предпочитать односторонние упражнения");
    }

    return joinOrNone(values, "нет");
}

[[nodiscard]] std::string formatExerciseKind(core::ExerciseKind kind) {
    switch (kind) {
        case core::ExerciseKind::Resistance: return "силовое";
        case core::ExerciseKind::Olympic: return "олимпийское";
        case core::ExerciseKind::Cardio: return "кардио";
        case core::ExerciseKind::Mobility: return "мобильность";
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatExerciseStructure(core::ExerciseStructure structure) {
    switch (structure) {
        case core::ExerciseStructure::Compound: return "многосуставное";
        case core::ExerciseStructure::Isolation: return "изолирующее";
    }
    return "неизвестно";
}

[[nodiscard]] std::string formatProfileSummary(const core::UserProfile& profile) {
    std::ostringstream out;
    out << "#" << profile.getId() << " " << profile.getName()
        << " — " << profile.getAge() << " лет, "
        << formatGender(profile.getGender()) << ", "
        << formatLevel(profile.getLevel()) << ", "
        << formatGoal(profile.getGoal()) << ", "
        << formatFrequency(profile.getFrequency());
    return out.str();
}

[[nodiscard]] std::string formatProfileDetails(const core::UserProfile& profile) {
    const core::BodyMetrics& metrics = profile.getBodyMetrics();

    std::ostringstream out;
    out << "#" << profile.getId() << " " << profile.getName() << '\n';
    out << "  Возраст: " << profile.getAge() << '\n';
    out << "  Пол: " << formatGender(profile.getGender()) << '\n';
    out << "  Рост: " << formatDouble(metrics.heightCm) << " см\n";
    out << "  Вес: " << formatDouble(metrics.weightKg) << " кг\n";
    out << "  Процент жира: ";
    if (metrics.bodyFatPct < 0.0) {
        out << "неизвестно\n";
    } else {
        out << formatDouble(metrics.bodyFatPct) << "%\n";
    }
    out << "  Тренировочный стаж: " << metrics.trainingAgeMonths << " мес\n";
    out << "  Уровень: " << formatLevel(profile.getLevel()) << '\n';
    out << "  Цель: " << formatGoal(profile.getGoal()) << '\n';
    out << "  Частота: " << formatFrequency(profile.getFrequency()) << '\n';
    out << "  ИМТ: " << formatDouble(metrics.bmi) << '\n';
    out << "  BMR: " << formatDouble(metrics.bmrKcal, 0) << " ккал\n";
    out << "  TDEE: " << formatDouble(metrics.tdeeKcal, 0) << " ккал\n";
    out << "  Целевые калории: " << formatDouble(profile.getTargetCalories(), 0)
        << " ккал\n";
    out << "  Оборудование: "
        << formatEquipmentMask(profile.getConstraints().availableEquipment) << '\n';
    out << "  Ограничения: " << formatHealthConstraints(profile.getConstraints())
        << '\n';
    out << "  Лимит сессии: ";
    if (profile.getConstraints().maxSessionMinutes) {
        out << *profile.getConstraints().maxSessionMinutes << " мин\n";
    } else {
        out << "не задан\n";
    }
    return out.str();
}

[[nodiscard]] std::string formatExerciseSummary(const core::Exercise& exercise) {
    std::ostringstream out;
    out << "#" << exercise.getId() << " " << exercise.getName()
        << " | " << formatMuscleGroup(exercise.getPrimaryMuscle())
        << " | " << formatEquipmentMask(exercise.getRequiredEquipment())
        << " | сложность " << static_cast<int>(exercise.getDifficulty())
        << " | " << formatExerciseKind(exercise.getKind());
    return out.str();
}

[[nodiscard]] std::string formatExerciseDetails(const core::Exercise& exercise) {
    std::ostringstream out;
    out << "#" << exercise.getId() << " " << exercise.getName() << '\n';
    out << "  Описание: " << exercise.getDescription() << '\n';
    out << "  Основная мышца: " << formatMuscleGroup(exercise.getPrimaryMuscle())
        << '\n';

    std::vector<std::string> secondary;
    for (const core::MuscleGroup muscle : exercise.getSecondaryMuscles()) {
        secondary.push_back(formatMuscleGroup(muscle));
    }
    out << "  Дополнительные мышцы: " << joinOrNone(secondary, "нет") << '\n';
    out << "  Структура: " << formatExerciseStructure(exercise.getStructure())
        << '\n';
    out << "  Тип: " << formatExerciseKind(exercise.getKind()) << '\n';
    out << "  Оборудование: "
        << formatEquipmentMask(exercise.getRequiredEquipment()) << '\n';
    out << "  Сложность: " << static_cast<int>(exercise.getDifficulty())
        << "/5\n";

    if (!exercise.getInstructions().empty()) {
        out << "  Инструкции: " << exercise.getInstructions() << '\n';
    }
    if (!exercise.getVideoUrl().empty()) {
        out << "  Видео: " << exercise.getVideoUrl() << '\n';
    }
    return out.str();
}

[[nodiscard]] std::string formatPlanExercise(const app::PlanExerciseInfo& exercise,
                                            std::size_t position) {
    std::ostringstream out;
    out << "  " << (position + 1) << ". " << exercise.exerciseName
        << " — " << exercise.sets << "x" << exercise.reps
        << ", отдых " << exercise.restSec << " с";

    if (exercise.weightKg) {
        out << ", вес " << formatDouble(*exercise.weightKg) << " кг";
    }
    if (!exercise.notes.empty()) {
        out << ", заметки: " << exercise.notes;
    }
    return out.str();
}

[[nodiscard]] std::string formatGeneratedExercise(
    const core::PlannedExercise& exercise,
    std::size_t position,
    const std::map<core::ExerciseId, std::string>& names) {
    const auto nameIt = names.find(exercise.exerciseId);
    const std::string name = nameIt != names.end()
                                 ? nameIt->second
                                 : "#" + std::to_string(exercise.exerciseId);

    std::ostringstream out;
    out << "  " << (position + 1) << ". " << name
        << " — " << exercise.sets << "x" << exercise.reps
        << ", отдых " << exercise.restSec << " с";
    return out.str();
}

[[nodiscard]] std::string formatPlanSummary(const app::PlanInfo& plan) {
    std::ostringstream out;
    out << "#" << plan.id << " " << plan.name
        << " — " << formatGoal(plan.goal)
        << ", дней: " << plan.daysPerWeek
        << ", " << (plan.isActive ? "активен" : "неактивен");
    return out.str();
}

[[nodiscard]] std::string formatPlanDetails(const app::PlanInfo& plan) {
    std::ostringstream out;
    out << "#" << plan.id << " " << plan.name << '\n';
    out << "  Цель: " << formatGoal(plan.goal) << '\n';
    out << "  Дней в неделю: " << plan.daysPerWeek << '\n';
    out << "  Статус: " << (plan.isActive ? "активен" : "неактивен") << '\n';
    if (!plan.notes.empty()) {
        out << "  Заметки: " << plan.notes << '\n';
    }

    for (const app::PlanDayInfo& day : plan.days) {
        out << "  День " << (day.dayIndex + 1);
        if (!day.name.empty()) {
            out << ": " << day.name;
        }
        out << '\n';

        if (day.exercises.empty()) {
            out << "    (нет упражнений)\n";
        }
        for (std::size_t i = 0; i < day.exercises.size(); ++i) {
            out << formatPlanExercise(day.exercises[i], i) << '\n';
        }
    }
    return out.str();
}

[[nodiscard]] std::string formatGeneratedPlan(
    const core::WorkoutPlan& plan,
    const std::map<core::ExerciseId, std::string>& exerciseNames) {
    std::ostringstream out;
    for (const core::PlanDay& day : plan.getDays()) {
        out << "День " << (day.dayIndex + 1);
        if (!day.name.empty()) {
            out << ": " << day.name;
        }
        out << '\n';

        if (day.exercises.empty()) {
            out << "  (нет упражнений)\n";
        }
        for (std::size_t i = 0; i < day.exercises.size(); ++i) {
            out << formatGeneratedExercise(day.exercises[i], i, exerciseNames)
                << '\n';
        }
    }
    return out.str();
}

[[nodiscard]] std::string formatOneRepMax(core::ExerciseId exerciseId,
                                          const std::string& exerciseName,
                                          double kg) {
    std::ostringstream out;
    out << "#" << exerciseId << " " << exerciseName << ": "
        << formatDouble(kg) << " кг";
    return out.str();
}

[[nodiscard]] std::string formatServiceError(const app::ServiceError& error) {
    if (error.code() == "PROFILE_NOT_FOUND") {
        return std::string(serviceProfileNotFound);
    }
    if (error.code() == "EXERCISE_NOT_FOUND") {
        return std::string(serviceExerciseNotFound);
    }
    if (error.code() == "EMPTY_PLAN") {
        return std::string(serviceEmptyPlan);
    }
    if (error.code() == "INVALID_ARGUMENT") {
        return std::string(serviceInvalidArgument);
    }
    return std::string(serviceUnknown) + " " + error.code() + ": " + error.what();
}

[[nodiscard]] std::vector<int> parseNumberList(std::string_view text) {
    std::vector<int> numbers;
    std::stringstream stream{std::string(text)};
    std::string token;

    while (std::getline(stream, token, ',')) {
        token = trimCopy(token);
        if (token.empty()) {
            continue;
        }

        std::size_t parsed = 0;
        const int value = std::stoi(token, &parsed);
        if (parsed != token.size()) {
            throw std::invalid_argument("invalid number list");
        }
        numbers.push_back(value);
    }
    return numbers;
}

[[nodiscard]] core::Equipment equipmentFromNumbers(const std::vector<int>& numbers) {
    std::uint16_t mask = 0;
    for (const int number : numbers) {
        if (number == 0) {
            continue;
        }
        if (number < 1 || number > 11) {
            throw std::invalid_argument("equipment number must be between 1 and 11");
        }
        mask |= static_cast<std::uint16_t>(1u << (number - 1));
    }
    return static_cast<core::Equipment>(mask);
}

[[nodiscard]] std::uint16_t healthMaskFromNumbers(const std::vector<int>& numbers) {
    std::uint16_t mask = 0;
    for (const int number : numbers) {
        if (number == 0) {
            continue;
        }
        if (number < 1 || number > 9) {
            throw std::invalid_argument("health constraint number must be between 1 and 9");
        }
        mask |= core::HC_MASK(static_cast<core::HealthConstraint>(number));
    }
    return mask;
}

[[nodiscard]] bool hasEmptyDay(const core::WorkoutPlan& plan) {
    for (const core::PlanDay& day : plan.getDays()) {
        if (day.exercises.empty()) {
            return true;
        }
    }
    return false;
}

} // namespace

ConsoleUI::ConsoleUI(app::WorkoutService& service, std::istream& in,
                     std::ostream& out)
    : service_(service), in_(in), out_(out) {}

void ConsoleUI::run() {
    out_ << appTitle << '\n';

    try {
        while (running_) {
            try {
                mainMenu();
            } catch (const InputClosed&) {
                throw;
            } catch (const app::ServiceError& error) {
                printServiceError(error);
            } catch (const std::exception& error) {
                printError(error.what());
            }
        }
    } catch (const InputClosed&) {
        out_ << '\n' << inputClosed << '\n';
    }
}

void ConsoleUI::mainMenu() {
    out_ << mainMenuText;

    if (selectedProfileId_) {
        const std::optional<core::UserProfile> profile =
            service_.getProfile(*selectedProfileId_);
        if (profile) {
            out_ << currentProfilePrefix << "#" << profile->getId() << " "
                 << profile->getName() << '\n';
        } else {
            selectedProfileId_.reset();
        }
    }

    const int choice = readInt(choicePrompt, 0, 4);
    switch (choice) {
        case 0:
            running_ = false;
            out_ << goodbye << '\n';
            break;
        case 1:
            profileMenu();
            break;
        case 2:
            catalogMenu();
            break;
        case 3:
            generatePlan();
            break;
        case 4:
            plansMenu();
            break;
        default:
            out_ << unknownCommand << '\n';
            break;
    }
}

void ConsoleUI::profileMenu() {
    while (true) {
        out_ << profileMenuText;
        const int choice = readInt(choicePrompt, 0, 6);

        switch (choice) {
            case 0: return;
            case 1: showProfiles(); break;
            case 2: createProfile(); break;
            case 3: selectProfile(); break;
            case 4: editProfile(); break;
            case 5: manageOneRepMaxes(); break;
            case 6: deleteProfile(); break;
            default: out_ << unknownCommand << '\n'; break;
        }
    }
}

void ConsoleUI::catalogMenu() {
    while (true) {
        out_ << catalogMenuText;
        const int choice = readInt(choicePrompt, 0, 3);

        switch (choice) {
            case 0:
                return;
            case 1:
                showCatalog(app::ExerciseFilter{});
                break;
            case 2: {
                app::ExerciseFilter filter;
                filter.nameContains = readLine(promptFilterName);
                showCatalog(filter);
                break;
            }
            case 3:
                showExerciseDetails();
                break;
            default:
                out_ << unknownCommand << '\n';
                break;
        }
    }
}

void ConsoleUI::plansMenu() {
    while (true) {
        out_ << plansMenuText;
        const int choice = readInt(choicePrompt, 0, 4);

        switch (choice) {
            case 0: return;
            case 1: showPlans(); break;
            case 2: viewPlan(); break;
            case 3: activatePlan(); break;
            case 4: deletePlan(); break;
            default: out_ << unknownCommand << '\n'; break;
        }
    }
}

void ConsoleUI::showProfiles() {
    const std::vector<core::UserProfile> profiles = service_.listProfiles();
    if (profiles.empty()) {
        out_ << noProfiles << '\n';
        return;
    }

    for (const core::UserProfile& profile : profiles) {
        out_ << formatProfileSummary(profile);
        if (selectedProfileId_ && profile.getId() == *selectedProfileId_) {
            out_ << selectedMarker;
        }
        out_ << '\n';
    }
}

void ConsoleUI::createProfile() {
    out_ << newProfileTitle;

    const std::string name = readNonEmptyLine(promptName);
    const int age = readOptionalInt(promptAge, 13, 100).value_or(30);
    const core::Gender gender = askGender(core::Gender::Male);
    const double height =
        readOptionalDouble(promptHeight, 50.1, 249.9).value_or(175.0);
    const double weight =
        readOptionalDouble(promptWeight, 20.1, 299.9).value_or(75.0);
    const double bodyFat =
        readOptionalDouble(promptBodyFat, 3.0, 50.0).value_or(-1.0);
    const int trainingAge =
        readOptionalInt(promptTrainingAge, 0, age * 12).value_or(0);
    const core::ExperienceLevel level = askLevel(core::ExperienceLevel::Novice);
    const core::TrainingGoal goal = askGoal(core::TrainingGoal::Hypertrophy);
    const core::TrainingFrequency frequency =
        askFrequency(core::TrainingFrequency::Days3);

    core::UserConstraints defaults;
    defaults.availableEquipment = allEquipment();
    const core::UserConstraints constraints = askConstraints(defaults);

    const std::optional<core::UserProfile> profile = core::UserProfile::create(
        name, age, gender, height, weight, level, goal, frequency, constraints,
        bodyFat, trainingAge);

    if (!profile) {
        printError(std::string(profileInvalid));
        return;
    }

    selectedProfileId_ = service_.saveProfile(*profile);
    out_ << profileCreated << *selectedProfileId_ << '\n';
}

void ConsoleUI::selectProfile() {
    showProfiles();
    if (service_.listProfiles().empty()) {
        return;
    }

    const int id = readInt(promptProfileId, 1, 1000000);
    const std::optional<core::UserProfile> profile = service_.getProfile(id);
    if (!profile) {
        printError(std::string(serviceProfileNotFound));
        return;
    }

    selectedProfileId_ = profile->getId();
    out_ << profileSelected << "#" << profile->getId() << " "
         << profile->getName() << '\n';
}

void ConsoleUI::editProfile() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    core::UserProfile profile = *selected;
    bool editing = true;

    while (editing) {
        out_ << formatProfileDetails(profile) << editProfileMenu;
        const int choice = readInt(choicePrompt, 0, 13);
        bool changed = false;

        switch (choice) {
            case 0:
                editing = false;
                break;
            case 1: {
                const std::string name = readLine(promptName);
                if (!name.empty()) {
                    profile = profile.withName(name);
                    changed = true;
                }
                break;
            }
            case 2: {
                const std::optional<int> age = readOptionalInt(promptAge, 13, 100);
                if (age) {
                    profile = profile.withAgeYears(*age);
                    changed = true;
                }
                break;
            }
            case 3:
                profile = profile.withGender(askGender(profile.getGender()));
                changed = true;
                break;
            case 4: {
                const std::optional<double> height =
                    readOptionalDouble(promptHeight, 50.1, 249.9);
                if (height) {
                    profile = profile.withHeightCm(*height);
                    changed = true;
                }
                break;
            }
            case 5: {
                const std::optional<double> weight =
                    readOptionalDouble(promptWeight, 20.1, 299.9);
                if (weight) {
                    profile = profile.withBodyWeight(*weight);
                    changed = true;
                }
                break;
            }
            case 6: {
                const std::optional<double> bodyFat =
                    readOptionalDouble(promptBodyFat, 3.0, 50.0);
                if (bodyFat) {
                    profile = profile.withBodyFatPct(*bodyFat);
                    changed = true;
                }
                break;
            }
            case 7: {
                const std::optional<int> trainingAge = readOptionalInt(
                    promptTrainingAge, 0, profile.getAge() * 12);
                if (trainingAge) {
                    profile = profile.withTrainingAgeMonths(*trainingAge);
                    changed = true;
                }
                break;
            }
            case 8:
                profile = profile.withLevel(askLevel(profile.getLevel()));
                changed = true;
                break;
            case 9:
                profile = profile.withGoal(askGoal(profile.getGoal()));
                changed = true;
                break;
            case 10:
                profile = profile.withFrequency(
                    askFrequency(profile.getFrequency()));
                changed = true;
                break;
            case 11: {
                core::UserConstraints constraints = profile.getConstraints();
                constraints.availableEquipment =
                    askEquipment(constraints.availableEquipment);
                profile = profile.withConstraints(constraints);
                changed = true;
                break;
            }
            case 12: {
                core::UserConstraints constraints = profile.getConstraints();
                constraints.constraintsMask =
                    askHealthMask(constraints.constraintsMask);
                profile = profile.withConstraints(constraints);
                changed = true;
                break;
            }
            case 13: {
                core::UserConstraints constraints = profile.getConstraints();
                constraints.maxSessionMinutes =
                    askMaxSession(constraints.maxSessionMinutes);
                profile = profile.withConstraints(constraints);
                changed = true;
                break;
            }
            default:
                out_ << unknownCommand << '\n';
                break;
        }

        if (changed) {
            service_.saveProfile(profile);
            out_ << profileSaved << '\n';
        }
    }
}

void ConsoleUI::manageOneRepMaxes() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    const int64_t userId = selected->getId();

    while (true) {
        out_ << "\n=== 1ПМ ===\n";
        const std::map<core::ExerciseId, double> maxes =
            service_.getOneRepMaxes(userId);
        if (maxes.empty()) {
            out_ << noOneRepMaxes << '\n';
        } else {
            for (const auto& [exerciseId, kg] : maxes) {
                const std::optional<core::Exercise> exercise =
                    service_.getExercise(exerciseId);
                const std::string name = exercise
                                             ? exercise->getName()
                                             : "#" + std::to_string(exerciseId);
                out_ << "  " << formatOneRepMax(exerciseId, name, kg) << '\n';
            }
        }

        out_ << oneRepMaxMenu;
        const int choice = readInt(choicePrompt, 0, 2);
        if (choice == 0) {
            return;
        }

        const int exerciseId = readInt(promptExerciseId, 1, 1000000);
        const std::optional<core::Exercise> exercise =
            service_.getExercise(exerciseId);
        if (!exercise) {
            printError(std::string(serviceExerciseNotFound));
            continue;
        }

        if (choice == 1) {
            const double kg = readDouble(promptOneRepMaxKg, 0.1, 1000.0);
            service_.setOneRepMax(userId, exerciseId, kg);
            out_ << oneRepMaxSaved << '\n';
        } else {
            service_.removeOneRepMax(userId, exerciseId);
            out_ << oneRepMaxRemoved << '\n';
        }
    }
}

void ConsoleUI::deleteProfile() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    if (!readYesNo(confirmDeleteProfile)) {
        return;
    }

    if (service_.deleteProfile(selected->getId())) {
        selectedProfileId_.reset();
        out_ << profileDeleted << '\n';
    } else {
        out_ << notFound << '\n';
    }
}

void ConsoleUI::showCatalog(const app::ExerciseFilter& filter) {
    const std::vector<core::Exercise> exercises = service_.listExercises(filter);
    if (exercises.empty()) {
        out_ << nothingFound << '\n';
        return;
    }

    for (const core::Exercise& exercise : exercises) {
        out_ << formatExerciseSummary(exercise) << '\n';
    }
}

void ConsoleUI::showExerciseDetails() {
    const int id = readInt(promptExerciseId, 1, 1000000);
    const std::optional<core::Exercise> exercise = service_.getExercise(id);
    if (!exercise) {
        printError(std::string(serviceExerciseNotFound));
        return;
    }

    out_ << formatExerciseDetails(*exercise);
}

void ConsoleUI::generatePlan() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    const core::WorkoutPlan plan = service_.generatePlan(*selected);
    if (plan.isEmpty()) {
        out_ << emptyPlanGenerated << '\n';
        return;
    }

    out_ << formatGeneratedPlan(plan, exerciseNames());
    if (hasEmptyDay(plan)) {
        out_ << emptyDaysWarning << '\n';
    }

    if (!readYesNo(savePlanPrompt)) {
        return;
    }

    const std::string name = readNonEmptyLine(promptPlanName);
    const bool active = readYesNo(makeActivePrompt);
    const int64_t planId = service_.saveGeneratedPlan(*selected, plan, name, active);
    out_ << planSaved << planId << '\n';
}

void ConsoleUI::showPlans() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    const std::vector<app::PlanInfo> plans = service_.listPlans(selected->getId());
    if (plans.empty()) {
        out_ << noPlans << '\n';
        return;
    }

    for (const app::PlanInfo& plan : plans) {
        out_ << formatPlanSummary(plan) << '\n';
    }
}

void ConsoleUI::viewPlan() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    const int id = readInt(promptPlanId, 1, 1000000);
    const std::optional<app::PlanInfo> plan = service_.getPlan(id);
    if (!plan || plan->userId != selected->getId()) {
        out_ << notFound << '\n';
        return;
    }

    out_ << formatPlanDetails(*plan);
}

void ConsoleUI::activatePlan() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    const int id = readInt(promptPlanId, 1, 1000000);
    const std::optional<app::PlanInfo> plan = service_.getPlan(id);
    if (!plan || plan->userId != selected->getId()) {
        out_ << notFound << '\n';
        return;
    }

    if (service_.activatePlan(id)) {
        out_ << planActivated << '\n';
    } else {
        out_ << notFound << '\n';
    }
}

void ConsoleUI::deletePlan() {
    const std::optional<core::UserProfile> selected = selectedProfile();
    if (!selected) {
        return;
    }

    const int id = readInt(promptPlanId, 1, 1000000);
    const std::optional<app::PlanInfo> plan = service_.getPlan(id);
    if (!plan || plan->userId != selected->getId()) {
        out_ << notFound << '\n';
        return;
    }

    if (!readYesNo(confirmDeletePlan)) {
        return;
    }

    if (service_.deletePlan(id)) {
        out_ << planDeleted << '\n';
    } else {
        out_ << notFound << '\n';
    }
}

std::optional<core::UserProfile> ConsoleUI::selectedProfile() {
    if (!selectedProfileId_) {
        out_ << noSelectedProfile << '\n';
        return std::nullopt;
    }

    const std::optional<core::UserProfile> profile =
        service_.getProfile(*selectedProfileId_);
    if (!profile) {
        selectedProfileId_.reset();
        out_ << serviceProfileNotFound << '\n';
        return std::nullopt;
    }
    return profile;
}

std::map<core::ExerciseId, std::string> ConsoleUI::exerciseNames() {
    std::map<core::ExerciseId, std::string> names;
    for (const core::Exercise& exercise : service_.listExercises()) {
        names.emplace(exercise.getId(), exercise.getName());
    }
    return names;
}

core::Gender ConsoleUI::askGender(core::Gender current) {
    const std::optional<int> value = readOptionalInt(promptGender, 1, 3);
    if (!value) {
        return current;
    }
    return static_cast<core::Gender>(*value - 1);
}

core::ExperienceLevel ConsoleUI::askLevel(core::ExperienceLevel current) {
    const std::optional<int> value = readOptionalInt(promptLevel, 1, 3);
    if (!value) {
        return current;
    }
    return static_cast<core::ExperienceLevel>(*value - 1);
}

core::TrainingGoal ConsoleUI::askGoal(core::TrainingGoal current) {
    const std::optional<int> value = readOptionalInt(promptGoal, 1, 4);
    if (!value) {
        return current;
    }
    return static_cast<core::TrainingGoal>(*value - 1);
}

core::TrainingFrequency ConsoleUI::askFrequency(
    core::TrainingFrequency current) {
    const std::optional<int> value = readOptionalInt(promptFrequency, 2, 6);
    if (!value) {
        return current;
    }
    return static_cast<core::TrainingFrequency>(*value);
}

core::Equipment ConsoleUI::askEquipment(core::Equipment current) {
    out_ << "Доступное оборудование:\n";
    for (int bit = 0; bit < 11; ++bit) {
        out_ << "  " << (bit + 1) << ". "
             << formatEquipment(static_cast<core::Equipment>(1u << bit)) << '\n';
    }
    out_ << "  0. Без оборудования\n";
    out_ << "Текущее: " << formatEquipmentMask(current) << '\n';

    const std::string line = readLine(promptEquipment);
    if (line.empty()) {
        return current;
    }
    return equipmentFromNumbers(parseNumberList(line));
}

std::uint16_t ConsoleUI::askHealthMask(std::uint16_t current) {
    out_ << "Ограничения здоровья:\n";
    for (int number = 1; number <= 9; ++number) {
        out_ << "  " << number << ". "
             << formatHealthConstraint(static_cast<core::HealthConstraint>(number))
             << '\n';
    }
    out_ << "  0. Нет ограничений\n";

    core::UserConstraints temp;
    temp.constraintsMask = current;
    out_ << "Текущие: " << formatHealthConstraints(temp) << '\n';

    const std::string line = readLine(promptHealth);
    if (line.empty()) {
        return current;
    }
    return healthMaskFromNumbers(parseNumberList(line));
}

std::optional<int> ConsoleUI::askMaxSession(
    const std::optional<int>& current) {
    out_ << "Текущий лимит сессии: ";
    if (current) {
        out_ << *current << " мин\n";
    } else {
        out_ << "не задан\n";
    }
    return readOptionalInt(promptMaxSession, 10, 300);
}

core::UserConstraints ConsoleUI::askConstraints(
    const core::UserConstraints& current) {
    core::UserConstraints constraints = current;
    constraints.availableEquipment =
        askEquipment(constraints.availableEquipment);
    constraints.constraintsMask = askHealthMask(constraints.constraintsMask);
    constraints.maxSessionMinutes =
        askMaxSession(constraints.maxSessionMinutes);
    return constraints;
}

std::string ConsoleUI::readLine(std::string_view prompt) {
    out_ << prompt;
    out_.flush();

    std::string line;
    if (!std::getline(in_, line)) {
        throw InputClosed(std::string(inputClosed));
    }
    return trimCopy(line);
}

std::string ConsoleUI::readNonEmptyLine(std::string_view prompt) {
    while (true) {
        std::string line = readLine(prompt);
        if (!line.empty()) {
            return line;
        }
        out_ << emptyValue << '\n';
    }
}

int ConsoleUI::readInt(std::string_view prompt, int min, int max) {
    while (true) {
        const std::string line = readLine(prompt);
        try {
            std::size_t parsed = 0;
            const int value = std::stoi(line, &parsed);
            if (parsed != line.size()) {
                throw std::invalid_argument("trailing characters");
            }
            if (value < min || value > max) {
                out_ << numberOutOfRange << '\n';
                continue;
            }
            return value;
        } catch (const std::exception&) {
            out_ << invalidNumber << '\n';
        }
    }
}

std::optional<int> ConsoleUI::readOptionalInt(std::string_view prompt, int min,
                                              int max) {
    while (true) {
        const std::string line = readLine(prompt);
        if (line.empty()) {
            return std::nullopt;
        }

        try {
            std::size_t parsed = 0;
            const int value = std::stoi(line, &parsed);
            if (parsed != line.size()) {
                throw std::invalid_argument("trailing characters");
            }
            if (value < min || value > max) {
                out_ << numberOutOfRange << '\n';
                continue;
            }
            return value;
        } catch (const std::exception&) {
            out_ << invalidNumber << '\n';
        }
    }
}

double ConsoleUI::readDouble(std::string_view prompt, double min, double max) {
    while (true) {
        const std::string line = readLine(prompt);
        try {
            std::size_t parsed = 0;
            const double value = std::stod(line, &parsed);
            if (parsed != line.size() || !std::isfinite(value)) {
                throw std::invalid_argument("invalid double");
            }
            if (value < min || value > max) {
                out_ << numberOutOfRange << '\n';
                continue;
            }
            return value;
        } catch (const std::exception&) {
            out_ << invalidNumber << '\n';
        }
    }
}

std::optional<double> ConsoleUI::readOptionalDouble(std::string_view prompt,
                                                    double min, double max) {
    while (true) {
        const std::string line = readLine(prompt);
        if (line.empty()) {
            return std::nullopt;
        }

        try {
            std::size_t parsed = 0;
            const double value = std::stod(line, &parsed);
            if (parsed != line.size() || !std::isfinite(value)) {
                throw std::invalid_argument("invalid double");
            }
            if (value < min || value > max) {
                out_ << numberOutOfRange << '\n';
                continue;
            }
            return value;
        } catch (const std::exception&) {
            out_ << invalidNumber << '\n';
        }
    }
}

bool ConsoleUI::readYesNo(std::string_view prompt) {
    while (true) {
        const std::string line = readLine(prompt);
        if (line == "д" || line == "да" || line == "Д" || line == "ДА" ||
            line == "Да" || line == "y" || line == "Y" || line == "yes" ||
            line == "Yes" || line == "YES" || line == "1") {
            return true;
        }
        if (line == "н" || line == "нет" || line == "Н" || line == "НЕТ" ||
            line == "Нет" || line == "n" || line == "N" || line == "no" ||
            line == "No" || line == "NO" || line == "0") {
            return false;
        }
        out_ << yesNoHint << '\n';
    }
}

void ConsoleUI::printError(const std::string& message) {
    out_ << errorPrefix << message << '\n';
}

void ConsoleUI::printServiceError(const app::ServiceError& error) {
    out_ << errorPrefix << formatServiceError(error) << '\n';
}

} // namespace workout::ui
