# LDF TimeSeries Cutter

**LDF TimeSeries Cutter** — это утилита для быстрой нарезки данных лазерной допплеровской флоуметрии экспортированных из программы LAZMA (LDF_3) основе заданных пользователем интервалов времени (минуты и секунды). Перед обработкой XSL-файлы должны быть сконвертированы в формат CSV.

## Основные возможности

*   Загрузка CSV-файла с временными рядами ЛДФ.
*   Выбор папки для сохранения нарезанных фрагментов.
*   Задание произвольного количества временных интервалов в формате "ММ:СС - ММ:СС".
*   Добавление и удаление интервалов.
*   Предварительный просмотр имен и путей выходных файлов, а также проверка корректности заданных интервалов.
*   Нарезка исходного CSV на несколько файлов, каждый из которых соответствует заданному интервалу.
*   Простой и понятный графический интерфейс.

## Скриншот

<!-- Добавьте сюда скриншот программы -->
![Screenshot](/assets/screenshot.png)
*Замените `screenshot.png` на реальное имя файла вашего скриншота.*

## Как использовать (для пользователей)

Эта инструкция предназначена для пользователей, использующих готовый исполняемый файл (`.exe`).

### Системные требования

*   Операционная система Windows (x64).
*   *Возможно*, потребуется установка [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170) (обычно последней версии для Visual Studio 2015-2022), если он еще не установлен в системе.

### Запуск программы

1.  Скачайте исполняемый файл [`LDF_TimeSeries_Cutter.exe`](LDF_TimeSeries_Cutter.exe).
2.  Запустите файл двойным кликом.

### Порядок работы

1.  **Выберите CSV-файл:**
    *   Нажмите кнопку "Выбрать CSV-файл".
    *   В появившемся диалоговом окне (которе можно растягивать) найдите и выберите ваш исходный CSV-файл.
        *   *Ожидаемый формат:* Файл должен иметь разделитель-точку с запятой (`;`), и первая колонка должна содержать временную метку в секундах (целых или дробных). Первая строка может быть заголовком (она будет скопирована в выходные файлы).
    *   После выбора файла путь к нему отобразится в интерфейсе. Рядом появится индикатор: `(OK)` если файл найден, или `(Ошибка!)` если возникла проблема.

2.  **Выберите папку сохранения:**
    *   Нажмите кнопку "Выбрать папку сохранения".
    *   В появившемся диалоговом окне (которе можно растягивать) выберите папку, куда будут сохраняться нарезанные файлы.
    *   Путь к папке отобразится в интерфейсе с индикатором `(OK)` или `(Ошибка!)`.
    *   *По умолчанию:* Если папка не выбрана или указанный путь некорректен, файлы будут сохраняться в ту же папку, где находится исходный CSV-файл.

3.  **Задайте временные интервалы:**
    *   Нажмите кнопку "+ Добавить фрагмент", чтобы создать новую строку для ввода интервала.
    *   Для каждого фрагмента введите время **Начала** и **Конца** в полях `Мин`:`Сек`.
    *   **Важно:** Время начала должно быть строго меньше времени конца (`Начало < Конец`). Секунды и минуты должны быть неотрицательными.
    *   Чтобы удалить ненужный фрагмент, нажмите кнопку `X` в соответствующей строке.

4.  **Предпросмотр выходных файлов:**
    *   В этом разделе вы увидите список файлов, которые будут созданы при нарезке.
    *   Для каждого *корректно заданного* фрагмента будет показано:
        *   Полный путь к выходному файлу (например, `output_folder/имя_файла_1.csv`).
        *   Временной диапазон в формате `(ММ:СС - ММ:СС)`.
    *   Если фрагмент задан *некорректно* (например, Начало >= Конец), он будет отмечен серым цветом и сообщением `(Ошибка: Начало >= Конец)`. Такие фрагменты будут проигнорированы при нарезке.

5.  **Начать нарезку:**
    *   Кнопка "Нарезать CSV" станет активной только тогда, когда:
        *   Выбран корректный CSV-файл.
        *   Выбрана корректная папка сохранения (или она будет использована по умолчанию).
        *   Задан хотя бы один *корректный* временной фрагмент (Начало < Конец).
    *   Нажмите кнопку "Нарезать CSV".
    *   Начнется процесс нарезки. В консольном окне (если оно видно) будет отображаться лог процесса. В зависимости от размера файла это может занять некоторое время.
    *   По завершении появится всплывающее окно:
        *   **"Нарезка Успешна"**: Если все выбранные *валидные* фрагменты были успешно обработаны. Сообщение укажет количество созданных файлов.
        *   **"Результат Нарезки"**: Если возникли ошибки или предупреждения (например, не удалось записать какой-то файл, или были пропущены невалидные интервалы). Внимательно прочитайте сообщение об ошибках.

### Выход из программы

Просто закройте окно программы.

## Сборка из исходного кода (для разработчиков)

### Зависимости

*   [CMake](https://cmake.org/) (версия 3.15 или выше)
*   Компилятор C++ с поддержкой C++17 (например, MSVC из Visual Studio, GCC, Clang)
*   Библиотеки (включены в репозиторий в папке `libs` или подключаются через CMake):
    *   [Dear ImGui](https://github.com/ocornut/imgui) (включая бэкенды для GLFW и OpenGL3)
    *   [GLFW](https://www.glfw.org/)
    *   [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)

### Инструкции по сборке (Пример)

1.  Клонируйте репозиторий.
2.  Создайте папку для сборки (например, `build`).
3.  Сконфигурируйте проект с помощью CMake:
    ```bash
    cd path/to/LDF-TimeSeries-Cutter
    cmake -S . -B build
    ```
    *Если GLFW не включен как подмодуль или не найден системой, CMake может выдать ошибку. Убедитесь, что зависимости настроены правильно в `CMakeLists.txt`.*
4.  Соберите проект:
    ```bash
    cmake --build build --config Release
    ```
5.  Исполняемый файл появится в папке `build/Release` (или аналогичной, в зависимости от генератора CMake).

## Лицензия

Этот проект распространяется под лицензией MIT. Это означает, что вы можете свободно использовать, копировать, изменять, распространять и продавать программное обеспечение, но **без каких-либо гарантий** ("as is"). Разработчик не несет ответственности за любой ущерб, возникший в результате использования программы. Полный текст лицензии см. в файле [LICENSE](LICENSE).

## Контакты

Лаборатория СПб ИВМР, Саргылана Ермолаева ([mailto:esagi@yandex.ru])