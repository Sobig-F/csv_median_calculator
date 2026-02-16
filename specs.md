# Техническая спецификация: CSV Median Calculator v1.0.0

## 1. Обзор системы

### 1.1 Назначение
Высокопроизводительное консольное приложение для вычисления медианного значения цен из CSV файлов в реальном времени с поддержкой многопоточности и memory-mapped files.

### 1.2 Ключевые требования
- **Производительность**: обработка >1 млн строк/сек
- **Потокобезопасность**: корректная работа в многопоточной среде
- **Эффективность памяти**: ограниченное потребление O(log n)
- **Режим реального времени**: отслеживание изменений файлов

## 2. Архитектура

### 2.1 Компоненты системы
```
   ┌─────────────────────────────────────────────────────────────┐
   │                                       CSV Median Calculator │
   ├───────────────┬───────────────┬───────────────┬─────────────┤
   │    CLI Parser │ Config Parser │        Logger │       Types │
   │    (argument_ │      (config_ │  (logger.hpp) │ (types.hpp) │
   │   parser.hpp) │   parser.hpp) │               │             │
   ├───────────────┴───────────────┴───────────────┴─────────────┤
   │                       Readers Manager (readers_manager.hpp) │
   │                     ┌─────────────┐                         │
   │                ┌────┤ CSV Reader  ├────┐                    │
   │                │    │ (csv_reader │    │                    │
   │                │    │ .hpp)       │    │                    │
   │                ▼    └─────────────┘    ▼                    │
   │          ┌─────────┐               ┌─────────┐              │
   │          │ Thread 1│      ...      │ Thread N│              │
   │          └─────────┘               └─────────┘              │
   ├─────────────────────────────────────────────────────────────┤
   │                                 Data Queue (data_queue.hpp) │
   ├─────────────────────────────────────────────────────────────┤
   │                   Median Calculator (median_calculator.hpp) │
   │                   ┌─────────────┐                           │
   │                   │ T-Digest    │                           │
   │                   │(tdigest.hpp)│                           │
   │                   └─────────────┘                           │
   ├─────────────────────────────────────────────────────────────┤
   │                           File Streamer (file_streamer.hpp) │
   └─────────────────────────────────────────────────────────────┘
```

### 2.2 Потоки выполнения

1. **Главный поток**: инициализация, управление
2. **Потоки-читатели**: N потоков (по числу файлов)
3. **Поток-калькулятор**: один поток для вычисления медианы

## 3. Детальная спецификация компонентов

### 3.1 Модуль парсинга аргументов (`argument_parser.hpp`)

**Структура `parsing_result`:**
```cpp
struct parsing_result {
    bool _streaming_mode{false};            // Режим реального времени
    bool _show_help{false};                 // Показать справку
    std::string _config_file{"config.toml"};// Путь к конфигу (по умолч. "config.toml")
    boost::program_options::variables_map _variables;
    std::vector<std::string> _extra_values; //extra-values
};
```
**Функции:**

`parse_arguments(int argc, char* argv[])` — парсинг аргументов

`create_options_description()` — создание описания опций

### 3.2 Модуль конфигурации (`config_parser.hpp`)
**Структура parsing_result:**
```cpp
struct parsing_result {
    path _input_dir;                            // Входная директория
    path _output_dir;                           // Выходная директория
    path _config_file;                          // Файл конфигурации
    string_vector _csv_files;                   // Список CSV файлов
    string_vector _csv_filename_mask;           // Маска имён файлов
    std::vector<std::string> _extra_values_name;// Список названий extra-values
};
```
**Поддерживаемый формат конфига (TOML):**

```toml
[main]
input = "path/to/input"
output = "path/to/output"           # опционально
filename_mask = ["mask1", "mask2"]  # опционально
```
### 3.3 Модуль типов данных (`types.hpp`)
**Класс data:**

```cpp
class data {
public:
    std::int_fast64_t receive_ts;  // Временная метка (наносекунды)
    double price;                   // Цена
    
    data(std::int_fast64_t time_, double price_) noexcept;
};
```
### 3.4 Потокобезопасная очередь (`data_queue.hpp`)
**Интерфейс:**

```cpp
class data_queue {
public:
    void push(std::unique_ptr<data> task_);                // Добавить задачу
    std::unique_ptr<data> wait_and_pop();                  // Извлечь (блокирующий)
    std::size_t size() const noexcept;                     // Размер очереди
    bool empty() const noexcept;                           // Проверка пустоты
    void stop() noexcept;                                  // Остановить ожидание
    std::atomic<std::size_t> total_count() const noexcept; // Всего задач
};
```
**Реализация:**

Мьютекс std::mutex для синхронизации

Condition variable std::condition_variable для ожидания

Атомарный флаг _stopped для корректного завершения

### 3.5 Читатель CSV (`csv_reader.hpp`)
**Технологии:**

- Memory-mapped files через Boost.Interprocess

- Отслеживание изменений файла в реальном времени

**Алгоритм чтения:**

- Отображение файла в память (file_mapping + mapped_region)

- Парсинг строк с разделителем ;

- Извлечение колонок receive_ts (индекс 0) и price (индекс 2)

- Отправка данных в очередь

- В режиме streaming: ожидание новых данных и повторное отображение

**Обработка ошибок:**

- Пропуск некорректных строк

- Логирование ошибок через spdlog

### 3.6 Менеджер читателей (`readers_manager.hpp`)
**Структура:**

```cpp
struct reader_thread_pair {
    std::shared_ptr<csv_reader> _reader;
    std::jthread _thread;  // C++20 jthread для автоматического join
};
```
**Функциональность:**

Создание потока на каждый CSV файл

Управление жизненным циклом потоков

Синхронизированное добавление новых файлов

### 3.7 T-Digest алгоритм (`tdigest.hpp`)
**Структура центроида:**

```cpp
struct centroid {
    double _mean;         // Среднее значение
    std::size_t _count;   // Количество точек
    
    void add(double value_) noexcept;
    void merge(const centroid& other_) noexcept;
};
```
**Алгоритм:**

- Добавление значения -> поиск ближайшего центроида

- Если вес центроида **НЕ** превышает лимит → объединение

    - Иначе создание нового центроида

- Периодическая компрессия при превышении лимита

**Математическая основа:**

- Квантильная оценка через интерполяцию между центроидами

- Весовой лимит: `max_weight(q) = 4 * compression * q * (1 - q)`

### 3.8 Калькулятор медианы (`median_calculator.hpp`)
**Параметры:**

`EPSILON = 1e-10` — порог изменения медианы

Компрессия T-Digest по умолчанию: `25`

**Логика работы:**

- Получение данных из очереди

- Обновление T-Digest

- Вычисление новой медианы

- Если изменение > EPSILON → запись в файл

### 3.9 Потоковый вывод (`file_streamer.hpp`)
**Формат вывода:**

```text
timestamp,median
<receive_ts>,<median_value>
```
**Особенности:**

- Автоматическое создание заголовка

- Буферизированная запись для производительности

- Возможность принудительного сброса (flush())

## 4. Протоколы и форматы
### 4.1 Входной CSV формат
Разделитель: `;` (точка с запятой)

Кодировка: `UTF-8`

Заголовок: `обязателен`

Колонки: `5`
```
receive_ts  — временная метка (int_fast64_t, наносекунды)
exchange_ts — (не используется)
price       — цена (double)
quantity    — (не используется)
side        — (не используется)
```
### 4.2 Выходной CSV формат
Разделитель: `;` (точка с запятой)

Заголовок: `timestamp;median`

Типы: `int_fast64_t, double`

## 5. Обработка ошибок
### 5.1 Коды возврата
`0` — успешное завершение

`1` — ошибка конфигурации

`1` — необработанное исключение

### 5.2 Логирование
Уровни логирования через spdlog:

`info` — информационные сообщения

`warn` — предупреждения

`error` — ошибки

`critical` — критические ошибки

### 5.3 Исключения
`std::invalid_argument` — неверные аргументы

`std::runtime_error` — ошибки времени выполнения

`boost::interprocess::interprocess_exception` — ошибки работы с файлами

## 6. Производительность и оптимизация
### 6.1 Метрики
**Пропускная способность:** target > 1 млн строк/сек

**Задержка:** < 100 мс от появления данных до записи

**Память:** < 50 МБ при любом размере входных данных

**CPU:** масштабирование по числу ядер

### 6.2 Оптимизации
**Memory-mapped files** — нулевое копирование при чтении

**T-Digest** — O(log n) память вместо O(n)

**Move semantics** — избегание копирования данных

**Lock-free очередь** — минимизация блокировок

**jthread** — автоматическое управление потоками

## 7. Сборка и зависимости
### 7.1 Зависимости
```
Библиотека	Версия	    Назначение
Boost	        1.84	    program_options, filesystem, interprocess
toml++	        3.4.0	    Парсинг TOML конфигов
spdlog	        1.x	    Логирование
C++23	        -	    Стандарт языка
```
### 7.2 CMake конфигурация
```cmake
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURE_DEPENDS true)

FetchContent_Declare(
    Boost
    GIT_REPOSITORY https://github.com/boostorg/boost.git
    GIT_TAG        boost-1.84.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Boost)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.x
)
FetchContent_MakeAvailable(spdlog)

set(BOOST_INCLUDE_LIBRARIES
    program_options
    filesystem
    interprocess
    regex
    algorithm
    lexical_cast
    accumulators
)

target_link_libraries(csv_median_calculator PRIVATE
    Boost::program_options
    Boost::filesystem
    Boost::regex
    Boost::interprocess
    Boost::algorithm
    Boost::lexical_cast
    Boost::accumulators
    tomlplusplus::tomlplusplus
    spdlog::spdlog
)
```
## 8. Тестирование
### 8.1 Модульные тесты (предполагаемые)
* Тест парсера аргументов

* Тест парсера конфигурации

* Тест T-Digest точности

* Тест потокобезопасности очереди

### 8.2 Интеграционные тесты
* Обработка 1 млн строк

* Режим реального времени с дописыванием файла

* Многопоточное чтение N файлов

### 8.3 Нагрузочное тестирование
* Максимальный размер файла: 10 ГБ

* Количество одновременных читателей: 16

* Длительность: 24 часа в streaming режиме

## 9. Ограничения
### 9.1 Текущие ограничения
* Только Windows (из-за chcp 65001 и Windows.h)

* Только CSV с разделителем `;`

* Фиксированный набор колонок (индексы 0 и 2)

### 9.2 Планируемые улучшения
* Кроссплатформенность (Linux/macOS)

* Поддержка пользовательского разделителя

* Конфигурация индексов колонок

* Сохранение состояния для длительных сессий