/**
 * logger.cpp
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


#include <stdlib.h>
#include "logger.h"

#ifdef SPINPLOG

namespace Spin
{

CLogger::CLogger()
    : m_inUse(false)
    , m_logs ()
{
}

thread_local CLogger::TMessageList*   CLogger::s_pPerThreadLogs          = nullptr;
thread_local CLogger::TCategoryIdMap* CLogger::s_pPerThreadCategoryIdMap = nullptr;
thread_local CLogger::TIdCategoryMap* CLogger::s_pPerThreadIdCategoryMap = nullptr;

thread_local int16_t  CLogger::s_categoryIdCounter = 0;
thread_local uint16_t CLogger::s_threadId          = 0;

thread_local std::string*    CLogger::s_pStreamCategory = nullptr;
thread_local std::string*    CLogger::s_pStreamMessage  = nullptr;
thread_local CLogger::ELevel CLogger::s_streamLevel     = CLogger::LEVEL_INFO;

std::atomic_uint_fast16_t CLogger::s_threadIdCounter(0);

CLogger::~CLogger()
{
}

void CLogger::flushStream()
{
    if (s_pStreamCategory->length() > 0 && s_pStreamMessage->length() > 0)
    {
        log(s_streamLevel, *s_pStreamCategory, *s_pStreamMessage);
    }

    s_pStreamMessage->clear();
    s_streamLevel = CLogger::LEVEL_INFO;
}

void CLogger::write(ELevel _level, const std::string& _rCategory, const std::string& _rMessage)
{
    CLogger::getInstance().log(_level, _rCategory, _rMessage);
}

void CLogger::write(const std::string& _rCategory, const std::string& _rMessage)
{
    CLogger::getInstance().log(_rCategory, _rMessage);
}

void CLogger::init()
{
    s_threadId = static_cast<uint16_t>(CLogger::s_threadIdCounter.fetch_add(1, std::memory_order_relaxed));

    s_pPerThreadLogs          = new TMessageList();
    s_pPerThreadCategoryIdMap = new TCategoryIdMap();
    s_pPerThreadIdCategoryMap = new TIdCategoryMap();
    s_pStreamCategory         = new std::string();
    s_pStreamMessage          = new std::string();
}

void CLogger::uninit()
{
    delete s_pPerThreadLogs         ;
    delete s_pPerThreadCategoryIdMap;
    delete s_pPerThreadIdCategoryMap;
    delete s_pStreamCategory        ;
    delete s_pStreamMessage         ;
}

void CLogger::openLogfile(const std::string& _rFilename)
{
}

void CLogger::closeFiles()
{
}

int16_t CLogger::createCategory(const std::string& _rCategory)
{
    int16_t id = CLogger::s_categoryIdCounter;
    CLogger::s_categoryIdCounter ++;

    std::string category = "prpl_spinp_" + _rCategory;

    s_pPerThreadCategoryIdMap->emplace(category, id);
    s_pPerThreadIdCategoryMap->emplace(id, category);

    return id;
}

void CLogger::log(ELevel _level, const std::string& _rCategory, const std::string& _rMessage)
{
    int16_t id;

    auto itCategory = s_pPerThreadCategoryIdMap->find(_rCategory);
    if (itCategory == s_pPerThreadCategoryIdMap->cend())
    {
        id = createCategory(_rCategory);
    }
    else
    {
        id = itCategory->second;
    }

    s_pPerThreadLogs->push_back(
        {
            CLogger::s_threadId,
            id,
            _level,
            _rMessage + "\n",
            std::chrono::system_clock::to_time_t(std::chrono::time_point<std::chrono::system_clock>())
        }
    );

    if (!m_inUse.exchange(true))
    {
        m_logs.splice(m_logs.end(), *s_pPerThreadLogs);

        auto itCategoriesPerThread = m_categories.find(CLogger::s_threadId);
        if (itCategoriesPerThread == m_categories.cend())
        {
            m_categories.emplace(CLogger::s_threadId, CLogger::TIdCategoryMap());
            itCategoriesPerThread = m_categories.find(CLogger::s_threadId);
        }

        itCategoriesPerThread->second = *s_pPerThreadIdCategoryMap;

        m_inUse.store(false);
    }
}

void CLogger::log(const std::string& _rCategory, const std::string& _rMessage)
{
    log(CLogger::LEVEL_INFO, _rCategory, _rMessage);
}

void CLogger::logMutexed(const std::string& _rFilename, const std::string& _rMessage)
{
    log(CLogger::LEVEL_INFO, _rFilename, _rMessage);
}

bool CLogger::acquireLogsLock()
{
    return !m_inUse.exchange(true);
}

void CLogger::freeLogsLock()
{
    m_logs.clear();

    m_inUse.store(false);
}

CLogger::TMessageList& CLogger::getLogs()
{
    return m_logs;
}

const CLogger::TThreadIdCategoryMap& CLogger::getCategories()
{
    return m_categories;
}

}

#else

namespace Spin
{
CLogger::CLogger()
{
}

CLogger::~CLogger()
{

}

void CLogger::flushStream()
{
}

void CLogger::write(ELevel _level, const std::string& _rCategory, const std::string& _rMessage)
{
}

void CLogger::write(const std::string& _rCategory, const std::string& _rMessage)
{
}

void CLogger::init()
{
}

void CLogger::uninit()
{
}

void CLogger::openLogfile(const std::string& _rFilename)
{
}

void CLogger::closeFiles()
{
}

int16_t CLogger::createCategory(const std::string& _rCategory)
{
    return -1;
}

void CLogger::log(ELevel _level, const std::string& _rCategory, const std::string& _rMessage)
{
}

void CLogger::log(const std::string& _rCategory, const std::string& _rMessage)
{
}

void CLogger::logMutexed(const std::string& _rFilename, const std::string& _rMessage)
{
}

bool CLogger::acquireLogsLock()
{
    return false;
}

void CLogger::freeLogsLock()
{
}

CLogger::TMessageList& CLogger::getLogs()
{
    static CLogger::TMessageList s_messagelist;

    return s_messagelist;
}

const CLogger::TThreadIdCategoryMap& CLogger::getCategories()
{
    static CLogger::TThreadIdCategoryMap s_idcatmap;

    return s_idcatmap;
}

}

#endif