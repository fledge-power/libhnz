#ifndef __CCRITICALSECTION_H__
#define __CCRITICALSECTION_H__


#include <pthread.h>

/// Classe de gestion des sections critiques
class CCriticalSection
{
    // Constructors
public:
    CCriticalSection();

    // Attributes
public:
    pthread_mutex_t	m_MUTEX;

    // Operations
    void Lock();
    void Unlock();

    // Implementation
public:
    virtual  ~CCriticalSection();
};


#endif // __CCRITICALSECTION_H__
