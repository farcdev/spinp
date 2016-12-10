/**
 * logger.h
 * Copyright (C) 2016 Farci <farci@rehpost.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <string>

#ifndef SPINTEST_LOGGER_H
#define SPINTEST_LOGGER_H

#include <map>
#include <fstream>
#include <mutex>
#ifdef __MINGW32__
#include <mingw.mutex.h>
#endif
#include <atomic>
#include <vector>
#include <unordered_map>
#include <list>
#include <chrono>

#ifdef SPINPLOG

namespace Spin
{

class CLogger
{
public:
    enum EEnd
    {
        end
    };

    enum ELevel
    {
        LEVEL_INFO,
        LEVEL_WARNING,
        LEVEL_ERROR,
        LEVEL_FATAL
    };

    struct SLogMessage
    {
        uint16_t     m_threadId;
        int16_t      m_category;
        ELevel       m_level;
        std::string  m_message;
        std::time_t  m_timeStamp;
    };

    typedef std::list<SLogMessage> TMessageList;
    typedef std::unordered_map<int16_t, std::string> TIdCategoryMap;
    typedef std::unordered_map<std::string, int16_t> TCategoryIdMap;
    typedef std::unordered_map<uint16_t, TIdCategoryMap> TThreadIdCategoryMap;

private:
    CLogger();
    ~CLogger();

public:
    inline static CLogger& getInstance()
    {
        static CLogger s_instance;

        return s_instance;
    }

    inline static CLogger& write(const char* _pCategory)
    {
        CLogger& rInstance = getInstance();

        rInstance.flushStream();
        s_pStreamCategory->assign(_pCategory);

        return rInstance;
    }

    inline static CLogger& info (const char* _pCategory) { CLogger& rInstance = write(_pCategory); rInstance.s_streamLevel = LEVEL_INFO   ; return rInstance; }
    inline static CLogger& warn (const char* _pCategory) { CLogger& rInstance = write(_pCategory); rInstance.s_streamLevel = LEVEL_WARNING; return rInstance; }
    inline static CLogger& error(const char* _pCategory) { CLogger& rInstance = write(_pCategory); rInstance.s_streamLevel = LEVEL_ERROR  ; return rInstance; }
    inline static CLogger& fatal(const char* _pCategory) { CLogger& rInstance = write(_pCategory); rInstance.s_streamLevel = LEVEL_FATAL  ; return rInstance; }

    inline static CLogger& write() { CLogger& rInstance = getInstance(); return rInstance; }

    static void write(ELevel _level, const std::string& _rCategory, const std::string& _rMessage);
    static void write(const std::string& _rCategory, const std::string& _rMessage);

    void init();
    void uninit();

    void openLogfile(const std::string& _rFilename);
    void closeFiles();
    void log(ELevel _level, const std::string& _rCategory, const std::string& _rMessage);
    void log(const std::string& _rCategory, const std::string& _rMessage);
    void logMutexed(const std::string& _rFilename, const std::string& _rMessage);

    bool                   acquireLogsLock();
    void                   freeLogsLock();
    TMessageList&          getLogs();
    const TThreadIdCategoryMap& getCategories();

public:
    inline CLogger& operator << (const std::string& _rString)
    {
        s_pStreamMessage->append(_rString);
        return *this;
    }

    inline CLogger& operator << (const char* _pString)
    {
        s_pStreamMessage->append(_pString);
        return *this;
    }

    inline CLogger& operator << (bool _bool)
    {
        s_pStreamMessage->append(_bool ? "TRUE" : "FALSE");
        return *this;
    }

    inline CLogger& operator << (char _char)
    {
        s_pStreamMessage->append(&_char, 1);
        return *this;
    }

//    inline CLogger& operator << (int8_t _integer)
//    {
//        s_pStreamMessage->append(std::to_string(_integer));
//        return *this;
//    }

    inline CLogger& operator << (int16_t _integer)
    {
        s_pStreamMessage->append(std::to_string(_integer));
        return *this;
    }

    inline CLogger& operator << (int32_t _integer)
    {
        s_pStreamMessage->append(std::to_string(_integer));
        return *this;
    }

    inline CLogger& operator << (int64_t _integer)
    {
        s_pStreamMessage->append(std::to_string(_integer));
        return *this;
    }

#ifdef __MINGW32__
    inline CLogger& operator << (long _integer)
    {
        s_pStreamMessage->append(std::to_string(_integer));
        return *this;
    }

    inline CLogger& operator << (unsigned long _integer)
    {
        s_pStreamMessage->append(std::to_string(_integer));
        return *this;
    }
#endif

    inline CLogger& operator << (uint8_t _unsignedInteger)
    {
        s_pStreamMessage->append(std::to_string(_unsignedInteger));
        return *this;
    }

    inline CLogger& operator << (uint16_t _unsignedInteger)
    {
        s_pStreamMessage->append(std::to_string(_unsignedInteger));
        return *this;
    }

    inline CLogger& operator << (uint32_t _unsignedInteger)
    {
        s_pStreamMessage->append(std::to_string(_unsignedInteger));
        return *this;
    }

    inline CLogger& operator << (uint64_t _unsignedInteger)
    {
        s_pStreamMessage->append(std::to_string(_unsignedInteger));
        return *this;
    }

    inline CLogger& operator << (float _float)
    {
        s_pStreamMessage->append(std::to_string(_float));
        return *this;
    }

    inline CLogger& operator << (double _double)
    {
        s_pStreamMessage->append(std::to_string(_double));
        return *this;
    }

    inline CLogger& operator << (CLogger::EEnd _end)
    {
        flushStream();
        return *this;
    }

private:
    void flushStream();

private:
    int16_t createCategory(const std::string& _rCategory);

private:
    thread_local static TMessageList*   s_pPerThreadLogs;
    thread_local static TCategoryIdMap* s_pPerThreadCategoryIdMap;
    thread_local static TIdCategoryMap* s_pPerThreadIdCategoryMap;
    thread_local static int16_t         s_categoryIdCounter;
    thread_local static uint16_t        s_threadId;

    thread_local static std::string* s_pStreamCategory;
    thread_local static std::string* s_pStreamMessage;
    thread_local static ELevel       s_streamLevel;

    static std::atomic_uint_fast16_t s_threadIdCounter;

    std::atomic_bool     m_inUse;
    TMessageList         m_logs;
    TThreadIdCategoryMap m_categories;

};


typedef CLogger Log;

}

#else

namespace Spin
{

class CLogger
{
public:
    enum EEnd
    {
        end
    };

    enum ELevel
    {
        LEVEL_INFO,
        LEVEL_WARNING,
        LEVEL_ERROR,
        LEVEL_FATAL
    };

    struct SLogMessage
    {
        uint16_t     m_threadId;
        int16_t      m_category;
        ELevel       m_level;
        std::string  m_message;
        std::time_t  m_timeStamp;
    };

    typedef std::list<SLogMessage> TMessageList;
    typedef std::unordered_map<int16_t, std::string> TIdCategoryMap;
    typedef std::unordered_map<std::string, int16_t> TCategoryIdMap;
    typedef std::unordered_map<uint16_t, TIdCategoryMap> TThreadIdCategoryMap;

private:
    CLogger();
    ~CLogger();

public:
    inline static CLogger& getInstance()
    {
        static CLogger s_instance;

        return s_instance;
    }

    inline static CLogger& write(const char* _pCategory)
    {
        CLogger& rInstance = getInstance();
        return rInstance;
    }

    inline static CLogger& info (const char* _pCategory) { CLogger& rInstance = getInstance(); return rInstance; }
    inline static CLogger& warn (const char* _pCategory) { CLogger& rInstance = getInstance(); return rInstance; }
    inline static CLogger& error(const char* _pCategory) { CLogger& rInstance = getInstance(); return rInstance; }
    inline static CLogger& fatal(const char* _pCategory) { CLogger& rInstance = getInstance(); return rInstance; }

    inline static CLogger& write() { CLogger& rInstance = getInstance(); return rInstance; }

    static void write(ELevel _level, const std::string& _rCategory, const std::string& _rMessage);
    static void write(const std::string& _rCategory, const std::string& _rMessage);

    void init();
    void uninit();

    void openLogfile(const std::string& _rFilename);
    void closeFiles();
    void log(ELevel _level, const std::string& _rCategory, const std::string& _rMessage);
    void log(const std::string& _rCategory, const std::string& _rMessage);
    void logMutexed(const std::string& _rFilename, const std::string& _rMessage);

    bool                   acquireLogsLock();
    void                   freeLogsLock();
    TMessageList&          getLogs();
    const TThreadIdCategoryMap& getCategories();

public:
    inline CLogger& operator << (const std::string& _rString)
    {
        return *this;
    }

    inline CLogger& operator << (const char* _pString)
    {
        return *this;
    }

    inline CLogger& operator << (bool _bool)
    {
        return *this;
    }

    inline CLogger& operator << (int8_t _integer)
    {
        return *this;
    }

    inline CLogger& operator << (int16_t _integer)
    {
        return *this;
    }

    inline CLogger& operator << (int32_t _integer)
    {
        return *this;
    }

    inline CLogger& operator << (int64_t _integer)
    {
        return *this;
    }

#ifdef __MINGW32__
    inline CLogger& operator << (long _integer)
    {
        return *this;
    }

    inline CLogger& operator << (unsigned long _integer)
    {
        return *this;
    }
#endif

    inline CLogger& operator << (uint8_t _unsignedInteger)
    {
        return *this;
    }

    inline CLogger& operator << (uint16_t _unsignedInteger)
    {
        return *this;
    }

    inline CLogger& operator << (uint32_t _unsignedInteger)
    {
        return *this;
    }

    inline CLogger& operator << (uint64_t _unsignedInteger)
    {
        return *this;
    }

    inline CLogger& operator << (float _float)
    {
        return *this;
    }

    inline CLogger& operator << (double _double)
    {
        return *this;
    }

    inline CLogger& operator << (CLogger::EEnd _end)
    {
        return *this;
    }

private:
    void flushStream();

private:
    int16_t createCategory(const std::string& _rCategory);

private:

};


typedef CLogger Log;

}

#endif // SPINPLOG

#endif // SPINTEST_LOGGER_H