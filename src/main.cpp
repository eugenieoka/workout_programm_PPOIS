#include <exception>
#include <iostream>
#include <string_view>

#include "app/AppOptions.h"
#include "app/DatabaseInitializer.h"
#include "app/WorkoutService.h"
#include "ui/ConsoleUI.h"

namespace {

constexpr std::string_view kHelpText = R"(Workout Planner — консольный интерфейс.

Использование:
  workout_cli [--db <path>] [--data-dir <path>] [--help]

Параметры:
  --db <path>        путь к файлу базы данных
  --data-dir <path>  каталог с schema.sql и seed.sql
  --help             показать эту справку

Переменные окружения:
  WORKOUT_DB         путь к базе данных
  WORKOUT_DATA_DIR   каталог с schema.sql и seed.sql
)";

constexpr std::string_view kErrorPrefix = "Ошибка: ";

} // namespace

int main(int argc, char** argv) {
    try {
        const workout::app::AppOptions options =
            workout::app::AppOptions::parse(argc, argv);

        if (options.showHelp) {
            std::cout << kHelpText;
            return 0;
        }

        workout::app::DatabaseInitializer initializer(options.dbPath,
                                                      options.dataDir);
        initializer.initialize();

        workout::app::WorkoutService service(initializer.database());
        workout::ui::ConsoleUI console(service, std::cin, std::cout);
        console.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << kErrorPrefix << error.what() << '\n';
        return 1;
    }
}
