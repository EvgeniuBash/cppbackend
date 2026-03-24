#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <thread>

using namespace std::literals;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    auto GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }

    auto GetTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);
        return std::put_time(std::localtime(&t_c), "%F %T");
    }

    void OpenFileIfNeeded() {
    std::string date = GetFileTimeStamp();

    if (date != current_file_date_) {
        if (log_file_.is_open()) {
            log_file_.close();
        }

        std::string filename = "/var/log/sample_log_" + date + ".log";
        log_file_.open(filename, std::ios::app);

        current_file_date_ = date;
        }
    }

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const {
        const auto now = GetTime();
        const auto t_c = std::chrono::system_clock::to_time_t(now);

        std::tm tm = *std::localtime(&t_c);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y_%m_%d");
        return oss.str();
    }

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        std::lock_guard lock(m_);

        OpenFileIfNeeded();

        if (!log_file_.is_open()) {
            return; 
        }
        
        log_file_ << GetTimeStamp() << ": "sv ;
        ((log_file_ << args), ...); 
        log_file_ << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts) {
        std::lock_guard<std::mutex> lock(m_);
        manual_ts_ = ts; 
    }

private:
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
    std::mutex m_;
    std::ofstream log_file_;
    std::string current_file_date_;
};
