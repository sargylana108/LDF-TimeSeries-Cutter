#include <windows.h> // Для SetConsoleOutputCP
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <chrono>
#include <iomanip> // Для setw/setfill в логах консоли
#include <cstdio>  // Для snprintf в предпросмотре

namespace fs = std::filesystem;

// --- СТРУКТУРА С МИНУТАМИ И СЕКУНДАМИ ---
struct TimeFragment {
    int start_min = 0;
    int start_sec = 0;
    int end_min = 0;
    int end_sec = 0;
    std::string id; // Уникальный ID для ImGui
};

// Функция для извлечения фрагмента CSV (без изменений, принимает double)
bool extract_fragment(const std::string& input_filename, const std::string& output_filename,
                     double start_time_sec, double end_time_sec) {
    std::ifstream infile(input_filename);
    if (!infile.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть входной файл: " << input_filename << std::endl;
        return false;
    }
    std::ofstream outfile(output_filename, std::ios::trunc);
    if (!outfile.is_open()) {
        std::cerr << "Ошибка: Не удалось открыть/создать выходной файл: " << output_filename << std::endl;
        infile.close();
        return false;
    }

    std::string line;
    bool header_written = false;
    bool in_range = false;

    try {
        while (std::getline(infile, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            if (!header_written) {
                outfile << line << std::endl;
                header_written = true;
                continue;
            }

            std::stringstream ss(line);
            std::string segment;
            double current_time_sec = -1.0;

            if (std::getline(ss, segment, ';')) {
                try {
                    size_t comma_pos = segment.find(',');
                    if (comma_pos != std::string::npos) segment[comma_pos] = '.';
                    current_time_sec = std::stod(segment);
                } catch (const std::invalid_argument& e) { continue; }
                  catch (const std::out_of_range& e) { continue; }
            } else { continue; }

            constexpr double epsilon = 1e-9; // Допуск для сравнения double
            if (current_time_sec >= (start_time_sec - epsilon) && current_time_sec <= (end_time_sec + epsilon)) {
                outfile << line << std::endl;
                in_range = true;
            } else if (in_range && current_time_sec > (end_time_sec + epsilon)) {
                 // Оптимизация (можно раскомментировать, если данные точно отсортированы)
                 // break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Непредвиденная ошибка при обработке файла: " << e.what() << std::endl;
        infile.close(); outfile.close(); return false;
    }

    infile.close(); outfile.close();

    if (!in_range) {
        // Форматируем вывод диапазона в MM:SS
        int start_m = static_cast<int>(start_time_sec / 60.0);
        int start_s = static_cast<int>(start_time_sec) % 60;
        int end_m = static_cast<int>(end_time_sec / 60.0);
        int end_s = static_cast<int>(end_time_sec) % 60;

        std::cerr << "Предупреждение: В файле " << input_filename
                  << " не найдено данных в диапазоне ["
                  << start_m << ":" << std::setw(2) << std::setfill('0') << start_s << " - "
                  << end_m << ":" << std::setw(2) << std::setfill('0') << end_s
                  << "]. Выходной файл " << output_filename << " может быть пустым..." << std::endl;
    }
    return true;
}

// Функция для отображения модального окна с сообщением (без изменений)
void ShowMessagePopup(const char* title, const std::string& message, const char* popup_id) {
    if (!ImGui::IsPopupOpen(popup_id)) {
        ImGui::OpenPopup(popup_id);
    }
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal(popup_id, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", message.c_str());
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}


int main() {
    // --- Установка кодировки консоли (оставляем) ---
    SetConsoleOutputCP(CP_UTF8);
    std::cout << "Кодировка консоли установлена на UTF-8." << std::endl;

    // --- Инициализация GLFW (оставляем) ---
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Можно оставить 3.3 или 3.0, как было
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1024, 768, "LDF TimeSeries Cutter", NULL, NULL); // Размер можно подстроить
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --- Инициализация ImGui (оставляем) ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    //ImGui::StyleColorsDark(); // Можно выбрать темный стиль
    ImGui::StyleColorsLight(); // ИЛИ Светлый стиль

    // --- Загрузка шрифта с кириллицей (оставляем улучшенную версию) ---
    ImFont* font = nullptr;
    const char* font_path_local = "tahoma.ttf"; // Поиск рядом с exe
    std::filesystem::path winFontsPath = "C:/Windows/Fonts";
    std::vector<fs::path> fontPathsToCheck = {
        fs::path(font_path_local),
        winFontsPath / "tahoma.ttf",
        winFontsPath / "verdana.ttf",
        winFontsPath / "arial.ttf"
    };
    bool fontLoaded = false;
    for (const auto& path : fontPathsToCheck) {
        if (fs::exists(path)) {
            font = io.Fonts->AddFontFromFileTTF(path.string().c_str(), 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            if (font) {
                std::cout << "Загружен шрифт: \"" << path.string() << "\"" << std::endl;
                fontLoaded = true;
                break;
            }
        }
    }
    if (!fontLoaded) {
        io.Fonts->AddFontDefault();
        std::cerr << "Шрифты с кириллицей не найдены. Используется стандартный шрифт ImGui." << std::endl;
    }
    io.Fonts->Build(); // Строим атлас

    // Инициализация бэкендов ПОСЛЕ настройки шрифтов
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core"); // Или #version 130 если нужно

    // --- Переменные состояния приложения ---
    std::string csvFilePath = "";
    std::string outputDirectoryPath = "";
    std::vector<TimeFragment> timeRanges; // ИСПОЛЬЗУЕМ НОВУЮ СТРУКТУРУ
    int nextFragmentId = 0;
    bool showErrorPopup = false;
    std::string currentErrorMessage = "";
    bool showSuccessPopup = false;
    std::string currentSuccessMessage = "";
    // Флаги валидности оставим, они полезны для кнопки "Нарезать"
    bool isCsvPathValid = false;
    bool isOutputDirValid = false;


    // --- Главный цикл приложения ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- Окно управления ---
        // Определение размера окна GLFW и установка размера окна ImGui
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));
        ImGui::Begin("LDF TimeSeries Cutter", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);


        // --- 1. Выбор файла (Оставляем как есть, с проверкой) ---
        if (ImGui::Button("Выбрать CSV-файл")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Выберите CSV файл", ".csv");
        }
        // Обработка диалога выбора файла и проверка валидности
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(400, 200))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                csvFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
                try {
                    isCsvPathValid = !csvFilePath.empty() && fs::exists(csvFilePath) && fs::is_regular_file(csvFilePath);
                } catch (const fs::filesystem_error& e) {
                    isCsvPathValid = false; currentErrorMessage = "Ошибка проверки файла: " + std::string(e.what()); showErrorPopup = true;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::SameLine();
        ImGui::TextWrapped("Файл: %s", csvFilePath.empty() ? "не выбран" : csvFilePath.c_str());
        if (!csvFilePath.empty()) {
            ImGui::SameLine();
            if (isCsvPathValid) ImGui::TextColored(ImVec4(0, 0.6f, 0, 1), "(OK)");
            else ImGui::TextColored(ImVec4(1, 0, 0, 1), "(Ошибка!)");
        }
        ImGui::Separator();


        // --- 2. Выбор папки вывода (Оставляем как есть, с проверкой) ---
        if (ImGui::Button("Выбрать папку сохранения")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseOutputDirDlgKey", "Выберите папку", nullptr);
        }
         // Обработка диалога выбора папки и проверка валидности
        if (ImGuiFileDialog::Instance()->Display("ChooseOutputDirDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(400, 200))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                outputDirectoryPath = ImGuiFileDialog::Instance()->GetCurrentPath();
                if (outputDirectoryPath.empty()) outputDirectoryPath = ImGuiFileDialog::Instance()->GetFilePathName();
                try {
                     if (!outputDirectoryPath.empty() && fs::exists(outputDirectoryPath) && !fs::is_directory(outputDirectoryPath)) {
                         outputDirectoryPath = fs::path(outputDirectoryPath).parent_path().string(); // Берем родителя, если выбрали файл
                     }
                    isOutputDirValid = !outputDirectoryPath.empty() && fs::exists(outputDirectoryPath) && fs::is_directory(outputDirectoryPath);
                } catch (const fs::filesystem_error& e) {
                    isOutputDirValid = false; currentErrorMessage = "Ошибка проверки папки: " + std::string(e.what()); showErrorPopup = true;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }
        ImGui::SameLine();
        ImGui::TextWrapped("Папка: %s", outputDirectoryPath.empty() ? "(по умолчанию: папка исходного CSV)" : outputDirectoryPath.c_str());
        if (!outputDirectoryPath.empty()) {
            ImGui::SameLine();
            if (isOutputDirValid) ImGui::TextColored(ImVec4(0, 0.6f, 0, 1), "(OK)");
            else ImGui::TextColored(ImVec4(1, 0, 0, 1), "(Ошибка!)");
        }
        ImGui::Separator();


        // --- 3. Управление временными интервалами (ФОРМАТ MM:SS) ---
        ImGui::Text("3. Задайте временные интервалы:");

        // Кнопка добавления нового интервала
        if (ImGui::Button("+ Добавить фрагмент")) {
            // Добавляем пустой фрагмент с уникальным ID
            timeRanges.push_back({0, 0, 0, 0, "##frag" + std::to_string(nextFragmentId++)});
        }
        ImGui::Separator();

        // Отображение и редактирование существующих интервалов
        int fragmentToRemove = -1; // Индекс фрагмента для удаления
        float input_width = 40.0f; // Ширина полей ввода мин/сек
        float spacing = 5.0f;      // Отступ между элементами

        for (size_t i = 0; i < timeRanges.size(); ++i) {
            ImGui::PushID(timeRanges[i].id.c_str()); // Уникальный ID для группы виджетов

            ImGui::Text("Фрагмент %zu:", i + 1); ImGui::SameLine(0, spacing*2);

            // --- Начало ---
            ImGui::Text("Начало:"); ImGui::SameLine(0, spacing);
            ImGui::PushItemWidth(input_width);
            ImGui::InputInt("##StartMin", &timeRanges[i].start_min, 0); // 0 = нет шага при +/-
            ImGui::PopItemWidth(); ImGui::SameLine();
            ImGui::Text(":"); ImGui::SameLine();
            ImGui::PushItemWidth(input_width);
            ImGui::InputInt("##StartSec", &timeRanges[i].start_sec, 0);
            ImGui::PopItemWidth(); ImGui::SameLine(0, spacing*3);

            // --- Конец ---
            ImGui::Text("Конец:"); ImGui::SameLine(0, spacing);
            ImGui::PushItemWidth(input_width);
            ImGui::InputInt("##EndMin", &timeRanges[i].end_min, 0);
            ImGui::PopItemWidth(); ImGui::SameLine();
            ImGui::Text(":"); ImGui::SameLine();
            ImGui::PushItemWidth(input_width);
            ImGui::InputInt("##EndSec", &timeRanges[i].end_sec, 0);
            ImGui::PopItemWidth(); ImGui::SameLine(0, spacing*3);

            // --- Кнопка удаления ---
            if (ImGui::Button("X")) {
                fragmentToRemove = static_cast<int>(i);
            }

            // Ограничение значений >= 0
            if (timeRanges[i].start_min < 0) timeRanges[i].start_min = 0;
            if (timeRanges[i].start_sec < 0) timeRanges[i].start_sec = 0;
            if (timeRanges[i].start_sec >= 60) timeRanges[i].start_sec = 59; // Опционально: ограничить секунды
            if (timeRanges[i].end_min < 0) timeRanges[i].end_min = 0;
            if (timeRanges[i].end_sec < 0) timeRanges[i].end_sec = 0;
            if (timeRanges[i].end_sec >= 60) timeRanges[i].end_sec = 59;   // Опционально: ограничить секунды

            ImGui::PopID(); // Завершение использования уникального ID
        }

        // Удаление фрагмента (если кнопка "X" была нажата)
        if (fragmentToRemove != -1) {
            timeRanges.erase(timeRanges.begin() + fragmentToRemove);
        }
        ImGui::Separator();


        // --- 4. Предпросмотр имен файлов (Обновленный вывод MM:SS) ---
        std::vector<std::pair<double, double>> validTimeRangesForCut; // Храним валидные диапазоны в секундах для нарезки
        bool hasInvalidFragments = false;

        if (!timeRanges.empty() && isCsvPathValid) {
             ImGui::Text("4. Предпросмотр выходных файлов:"); ImGui::Spacing();
             fs::path inputPath(csvFilePath);
             std::string baseName = inputPath.stem().string();
             std::string extension = inputPath.extension().string();

             // Определяем папку для предпросмотра
             std::string previewDir;
             if (isOutputDirValid) previewDir = outputDirectoryPath;
             else { previewDir = inputPath.parent_path().string(); if (previewDir.empty()) previewDir = "."; }

             validTimeRangesForCut.clear(); // Очищаем перед заполнением

             ImGui::Indent();
             for (size_t i = 0; i < timeRanges.size(); ++i) {
                 double start_time_total_sec = static_cast<double>(timeRanges[i].start_min) * 60.0 + timeRanges[i].start_sec;
                 double end_time_total_sec = static_cast<double>(timeRanges[i].end_min) * 60.0 + timeRanges[i].end_sec;

                 std::string outputFileNameOnly = baseName + "_" + std::to_string(i + 1) + extension;
                 fs::path previewPath = fs::path(previewDir) / outputFileNameOnly;
                 std::string previewPathStr = previewPath.string();
                 char buffer[512]; // Буфер для форматированного вывода

                 if (start_time_total_sec < end_time_total_sec) {
                     // Валидный диапазон
                     validTimeRangesForCut.push_back({start_time_total_sec, end_time_total_sec}); // Сохраняем для нарезки
                     // Форматируем вывод MM:SS - MM:SS
                     snprintf(buffer, sizeof(buffer),"%s (%d:%02d - %d:%02d)",
                              previewPathStr.c_str(),
                              timeRanges[i].start_min, timeRanges[i].start_sec,
                              timeRanges[i].end_min, timeRanges[i].end_sec);
                     ImGui::BulletText("%s", buffer);
                 } else {
                    // Невалидный диапазон
                    snprintf(buffer, sizeof(buffer),"%s (Ошибка: Начало >= Конец)", previewPathStr.c_str());
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    ImGui::Bullet(); ImGui::SameLine();
                    ImGui::TextWrapped("%s", buffer); // Wrapped на случай длинного пути
                    ImGui::PopStyleColor();
                    hasInvalidFragments = true;
                 }
             }
              if (validTimeRangesForCut.empty() && !timeRanges.empty()) {
                  ImGui::TextColored(ImVec4(0.8f, 0.5f, 0, 1), "Нет корректно заданных временных фрагментов для нарезки.");
              }
             ImGui::Unindent();
             ImGui::Spacing();
        }
        ImGui::Separator();


       // --- 5. Кнопка запуска нарезки ---
       ImGui::Text("5. Начать нарезку:");
       // Условие активации кнопки: выбран валидный CSV, выбрана валидная папка И есть хотя бы один валидный диапазон
       bool canCut = isCsvPathValid && isOutputDirValid && !validTimeRangesForCut.empty();

        if (!canCut) { ImGui::BeginDisabled(); } // Делаем неактивной, если нельзя резать

        if (ImGui::Button("Нарезать CSV", ImVec2(150, 30))) { // Кнопка покрупнее
            if (canCut) {
                // Сброс сообщений
                showErrorPopup = false; showSuccessPopup = false;
                currentErrorMessage = ""; currentSuccessMessage = "";
                std::string processErrors = ""; bool successOccurred = false; int files_written_count = 0;

                fs::path inputPath(csvFilePath);
                std::string baseName = inputPath.stem().string();
                std::string extension = inputPath.extension().string();
                std::string outputDir = outputDirectoryPath; // Папка уже проверена на isOutputDirValid

                std::cout << "--- Начало нарезки файла: " << csvFilePath << " ---" << std::endl;
                std::cout << "Папка для сохранения: " << outputDir << std::endl;

                // --- ИСПОЛЬЗУЕМ validTimeRangesForCut, собранный на этапе предпросмотра ---
                for (size_t i = 0; i < validTimeRangesForCut.size(); ++i) {
                    double start_time_sec = validTimeRangesForCut[i].first;
                    double end_time_sec = validTimeRangesForCut[i].second;

                    // Формируем имя файла (индекс i совпадает с порядком в validTimeRangesForCut)
                    std::string outputFileName = baseName + "_" + std::to_string(i + 1) + extension;
                    fs::path outputPath = fs::path(outputDir) / outputFileName;
                    std::string outputFileNameStr = outputPath.string();

                    // Вывод в консоль в формате MM:SS
                    int start_m = static_cast<int>(start_time_sec / 60.0); int start_s = static_cast<int>(start_time_sec) % 60;
                    int end_m = static_cast<int>(end_time_sec / 60.0);     int end_s = static_cast<int>(end_time_sec) % 60;
                    std::cout << "Нарезка фрагмента " << (i + 1) << ": "
                              << start_m << ":" << std::setw(2) << std::setfill('0') << start_s << " - "
                              << end_m << ":" << std::setw(2) << std::setfill('0') << end_s
                              << " -> " << outputFileNameStr << std::endl;

                    // Вызов функции нарезки
                    if (!extract_fragment(csvFilePath, outputFileNameStr, start_time_sec, end_time_sec)) {
                        processErrors += "Ошибка при обработке фрагмента " + std::to_string(i + 1) + " -> " + outputFileName + "\n";
                        std::cerr << "Ошибка записи файла: " << outputFileNameStr << std::endl;
                    } else {
                        successOccurred = true; files_written_count++;
                        std::cout << "Фрагмент " << (i + 1) << " успешно записан." << std::endl;
                    }
                }
                 std::cout << "--- Нарезка завершена ---" << std::endl;

                // Показ результатов (логика без изменений)
                // Добавляем сообщение о невалидных фрагментах, если они были, но нарезка все равно прошла
                 if (hasInvalidFragments) {
                      processErrors += "Примечание: Некоторые заданные фрагменты были невалидны (Начало >= Конец) и пропущены.\n";
                 }

                // Отладочный вывод
                #ifdef _DEBUG // Или любое другое имя макроса, например DEBUG_OUTPUT
                     std::cout << "DEBUG: Содержимое processErrors перед проверкой: [" << processErrors << "]" << std::endl;
                     std::cout << "DEBUG: Значение successOccurred: " << (successOccurred ? "true" : "false") << std::endl;
                     std::cout << "DEBUG: Количество записанных файлов: " << files_written_count << std::endl;
                #endif
     

                if (!processErrors.empty()) {
                     if (!processErrors.empty() && processErrors.back() == '\n') processErrors.pop_back();
                    currentErrorMessage = (successOccurred ? "Нарезка завершена с ошибками/предупреждениями:\n" : "Нарезка завершилась с ошибками:\n") + processErrors;
                     if (files_written_count > 0) currentErrorMessage += "\nПри этом успешно сохранено " + std::to_string(files_written_count) + " фрагмент(ов).";
                     else if (!successOccurred) currentErrorMessage += "\nНи одного фрагмента не было успешно сохранено.";
                    showErrorPopup = true;
                } else if (successOccurred) {
                    currentSuccessMessage = "Успешно сохранено " + std::to_string(files_written_count) + " фрагмент(ов).";
                    showSuccessPopup = true;
                } else {
                     currentErrorMessage = "Неизвестная ошибка: нет ни ошибок, ни успеха.";
                     showErrorPopup = true;
                }
            }
        }

       if (!canCut) {
            ImGui::EndDisabled();
             if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                 std::string tooltip_text = "Для нарезки необходимо:\n";
                 if (!isCsvPathValid) tooltip_text += "- Выбрать существующий CSV файл\n";
                 if (!isOutputDirValid) tooltip_text += "- Выбрать существующую папку для сохранения\n";
                 if (timeRanges.empty()) tooltip_text += "- Добавить хотя бы один временной фрагмент\n";
                 if (!timeRanges.empty() && validTimeRangesForCut.empty()) tooltip_text += "- Задать хотя бы один корректный временной фрагмент (начало < конец)\n";
                 ImGui::SetTooltip("%s", tooltip_text.c_str());
            }
       }


       ImGui::End(); // --- Конец окна ImGui ---


        // --- Отображение модальных окон сообщений (без изменений) ---
        if (showErrorPopup) {
            ShowMessagePopup("Результат Нарезки", currentErrorMessage, "ErrorPopup");
            if (!ImGui::IsPopupOpen("ErrorPopup")) showErrorPopup = false; // Сбрасываем флаг, когда окно закрывается
        }
        if (showSuccessPopup) {
            ShowMessagePopup("Нарезка Успешна", currentSuccessMessage, "SuccessPopup");
            if (!ImGui::IsPopupOpen("SuccessPopup")) showSuccessPopup = false; // Сбрасываем флаг, когда окно закрывается
        }

        // --- Рендеринг (без изменений) ---
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        // Устанавливаем цвет фона в зависимости от выбранной темы
        ImVec4 clear_color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg]; // Берем цвет фона окна ImGui
        // Можно немного осветлить/затемнить для фона вне окна ImGui, если нужно
        // Пример: clear_color = ImVec4(clear_color.x * 0.9f, clear_color.y * 0.9f, clear_color.z * 0.9f, 1.0f);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // --- Завершение работы (без изменений) ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
