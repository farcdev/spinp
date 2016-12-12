//
// Created by flex on 07.12.16.
//

#ifndef SPINTEST_JOB_H
#define SPINTEST_JOB_H

namespace Spin
{

class CConnector;

namespace Job
{

struct SJob
{
    virtual ~SJob()
    { }

private:
    virtual bool run(CConnector* _pConnector) = 0;

    friend class CJob;
};

struct SAuthAcceptBuddy : public SJob
{
    std::string m_userName;

private:
    virtual bool run(CConnector* _pConnector);
};

struct SAuthDeclineBuddy : public SJob
{
    std::string m_userName;

private:
    virtual bool run(CConnector* _pConnector);
};

struct SAskAuthBuddy : public SJob
{
    std::string m_userName;
    std::string m_message;

private:
    virtual bool run(CConnector* _pConnector);
};

struct SGetRoomList : public SJob
{
private:
    virtual bool run(CConnector* _pConnector);
};

struct SRefreshBuddyList : public SJob
{

private:
    virtual bool run(CConnector* _pConnector);
};

struct SUpdateUserInfo : public SJob
{
    std::string m_userName;
    std::string m_buddyImageHash;
    bool m_isBuddy;

private:
    virtual bool run(CConnector* _pConnector);
};

struct SRequestBuddyImage : public SJob
{
    std::string m_url;
    std::string m_userName;

private:
    virtual bool run(CConnector* _pConnector);
};

struct SRequestUserProfile : public SJob
{
    std::string m_userName;

private:
    virtual bool run(CConnector* _pConnector);
};

struct SLoadLoggedInPage : public SJob
{
private:
    virtual bool run(CConnector* _pConnector);
};

class CJob
{
public:
    template<class T>
    CJob(const T& _rJob);

    template<class T>
    CJob(T&& _rJob);

    virtual ~CJob();

    bool run(CConnector* _pConnector);

private:
    CJob();

private:
    SJob* m_pJob;
};

template<class T>
CJob::CJob(T&& _rJob)
    : m_pJob(new T(_rJob))
{
    static_assert(std::is_base_of<SJob, T>::value, "T must inherit from SJob");
}

template<class T>
CJob::CJob(const T& _rJob)
    : m_pJob(new T(_rJob))
{
    static_assert(std::is_base_of<SJob, T>::value, "T must inherit from SJob");
}

//template <class TConn, class T>
//inline void pushJob(TConn* _pConnector, T&& _rJob)
//{
//    _pConnector->pushJob(std::forward<T>(_rJob));
//}
//
//template <class TConn, class T>
//inline void pushJob(TConn* _pConnector, const T& _rJob)
//{
//    _pConnector->pushJob(_rJob);
//}
} // namespace Job
} // namespace Spin

#endif // SPINTEST_JOB_H
