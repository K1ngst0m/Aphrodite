//
// Created by npchitman on 6/22/21.
//

#ifndef HAZEL_ENGINE_INSTRUMENTOR_H
#define HAZEL_ENGINE_INSTRUMENTOR_H

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>

namespace Hazel {
    using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

    struct ProfileResult {
        std::string Name;
        FloatingPointMicroseconds Start;
        std::chrono::microseconds ElapsedTime;
        std::thread::id ThreadID;
    };

    struct InstrumentationSession {
        std::string Name;
    };

    class Instrumentor {
    public:
        Instrumentor(const Instrumentor&) = delete;
        Instrumentor(Instrumentor&&) = delete;

        void BeginSession(const std::string& name, const std::string& filepath = "results.json") {
            std::lock_guard lock(m_Mutex);
            if (m_CurrentSession) {
                if (Log::GetCoreLogger()) {
                    HZ_CORE_ERROR("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_CurrentSession->Name);
                }
                InternalEndSession();
            }
            m_OutputStream.open(filepath);
            if (m_OutputStream.is_open()) {
                m_CurrentSession = new InstrumentationSession{name};
                WriteHeader();
            } else {
                if (Log::GetCoreLogger()) {
                    HZ_CORE_ERROR("Instrumentor could not open results file '{0}'.", filepath);
                }
            }
        }

        void EndSession() {
            std::lock_guard lock(m_Mutex);
            InternalEndSession();
        }

        void WriteProfile(const ProfileResult& result) {
            std::stringstream json;

            json << "{";
            json << R"("cat":"function",)";
            json << R"("dur":)" << (result.ElapsedTime.count()) << ',';
            json << R"("name":")" << result.Name << "\",";
            json << R"("ph":"X",)";
            json << R"("pid":0,)";
            json << R"("tid":)" << result.ThreadID << ",";
            json << R"("ts":)" << result.Start.count();
            json << "}";

            std::lock_guard lock(m_Mutex);
            if (m_CurrentSession) {
                m_OutputStream << json.str();
                m_OutputStream.flush();
            }
        }

        static Instrumentor& Get() {
            static Instrumentor instance;
            return instance;
        }

    private:
        void WriteHeader() {
            m_OutputStream << R"({"otherData": {}, "traceEvents":[{})";
            m_OutputStream.flush();
        }

        void WriteFooter() {
            m_OutputStream << "]}";
            m_OutputStream.flush();
        }

        void InternalEndSession() {
            if (m_CurrentSession) {
                WriteFooter();
                m_OutputStream.close();
                delete m_CurrentSession;
                m_CurrentSession = nullptr;
            }
        }
    private:
        std::mutex m_Mutex;
        InstrumentationSession* m_CurrentSession;
        std::ofstream m_OutputStream;

    private:
        Instrumentor() : m_CurrentSession(nullptr) {}
        ~Instrumentor() { EndSession(); }

    };

    class InstrumentationTimer {
    public:
        explicit InstrumentationTimer(const char* name) : m_Name(name),
                                                          m_Stopped(m_Stopped) {
            m_StartTimepoint = std::chrono::steady_clock::now();
        }

        ~InstrumentationTimer() {
            if (!m_Stopped)
                Stop();
        }

        void Stop() {
            auto endTimepoint = std::chrono::high_resolution_clock::now();
            auto highResStart = FloatingPointMicroseconds{m_StartTimepoint.time_since_epoch()};
            auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

            Instrumentor::Get().WriteProfile({m_Name, highResStart, elapsedTime, std::this_thread::get_id()});

            m_Stopped = true;
        }

    private:
        const char* m_Name;
        std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
        bool m_Stopped;
    };

    namespace InstrumentorUtils {

        template<size_t N>
        struct ChangeResult {
            char Data[N];
        };

        template<size_t N, size_t K>
        constexpr auto CleanupOutputString(const char (&expr)[N], const char (&remove)[K]) {
            ChangeResult<N> result = {};

            size_t srcIndex = 0;
            size_t dstIndex = 0;
            while (srcIndex < N) {
                size_t matchIndex = 0;
                while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
                    matchIndex++;
                if (matchIndex == K - 1)
                    srcIndex += matchIndex;
                result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
                srcIndex++;
            }
            return result;
        }
    }// namespace InstrumentorUtils
}// namespace Hazel


#define HZ_PROFILE 1
#if HZ_PROFILE
// Resolve which function signature macro will be used. Note that this only
// is resolved when the (pre)compiler starts, so the syntax highlighting
// could mark the wrong one in your editor!
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define HZ_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define HZ_FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define HZ_FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define HZ_FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define HZ_FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define HZ_FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define HZ_FUNC_SIG __func__
#else
#define HZ_FUNC_SIG "HZ_FUNC_SIG unknown!"
#endif
#define HZ_PROFILE_BEGIN_SESSION(name, filepath) ::Hazel::Instrumentor::Get().BeginSession(name, filepath)
#define HZ_PROFILE_END_SESSION() ::Hazel::Instrumentor::Get().EndSession()
#define HZ_PROFILE_SCOPE_LINE2(name, line)                                                              \
    constexpr auto fixedName##line = ::Hazel::InstrumentorUtils::CleanupOutputString(name, "__cdecl "); \
    ::Hazel::InstrumentationTimer timer##line(fixedName##line.Data)
#define HZ_PROFILE_SCOPE_LINE(name, line) HZ_PROFILE_SCOPE_LINE2(name, line)
#define HZ_PROFILE_SCOPE(name) HZ_PROFILE_SCOPE_LINE(name, __LINE__)
#define HZ_PROFILE_FUNCTION() HZ_PROFILE_SCOPE(HZ_FUNC_SIG)
#else
#define HZ_PROFILE_BEGIN_SESSION(name, filepath)
#define HZ_PROFILE_END_SESSION()
#define HZ_PROFILE_SCOPE(name)
#define HZ_PROFILE_FUNCTION()
#endif

#endif//HAZEL_ENGINE_INSTRUMENTOR_H
