
#include "globus_thread_rmutex.h"

#ifndef BUILD_LITE

int
globus_rmutex_init(
    globus_rmutex_t *                   rmutex,
    globus_rmutexattr_t *               rattr)
{
    int                                 rc;
    
    rc = globus_mutex_init(&rmutex->mutex, GLOBUS_NULL);
    if(rc)
    {
        goto err_init;
        
    }
    rc = globus_cond_init(&rmutex->cond, GLOBUS_NULL);
    if(rc)
    {
        goto err_cond;
        
    }
    
    rmutex->level = 0;
    #ifdef WIN32
    memset(&rmutex->thread_id,0,sizeof(rmutex->thread_id));
    #else
    rmutex->thread_id = 0;
    #endif
    rmutex->waiting = 0;
    
    return 0;

err_cond:
    globus_mutex_destroy(&rmutex->mutex);
err_init:
    return rc;
}

int
globus_rmutex_lock(
    globus_rmutex_t *                   rmutex)
{
    globus_thread_t                     thread_id;
    
    thread_id = globus_thread_self();
    
    globus_mutex_lock(&rmutex->mutex);
    {
        globus_assert(rmutex->level >= 0);
        
        if(rmutex->level > 0 &&
            !globus_thread_equal(rmutex->thread_id, thread_id))
        {
            rmutex->waiting++;
            do
            {
                globus_cond_wait(&rmutex->cond, &rmutex->mutex);
            } while(rmutex->level > 0);
            rmutex->waiting--;
        }

        rmutex->level++;
        rmutex->thread_id = thread_id;
    }
    globus_mutex_unlock(&rmutex->mutex);
    
    return 0;
}

int
globus_rmutex_unlock(
    globus_rmutex_t *                   rmutex)
{
    globus_mutex_lock(&rmutex->mutex);
    {
        globus_assert(rmutex->level > 0);
        globus_assert(
            globus_thread_equal(rmutex->thread_id, globus_thread_self()));

        rmutex->level--;
        if(rmutex->level == 0)
        {
            #ifdef WIN32
            memset(&rmutex->thread_id,0,sizeof(rmutex->thread_id));
            #else
            rmutex->thread_id = 0;
            #endif
            if(rmutex->waiting)
            {
                globus_cond_signal(&rmutex->cond);
            }
        }
    }
    globus_mutex_unlock(&rmutex->mutex);
    
    return 0;
}

int
globus_rmutex_destroy(
    globus_rmutex_t *                   rmutex)
{
    globus_mutex_destroy(&rmutex->mutex);
    globus_cond_destroy(&rmutex->cond);
    
    return 0;
}

#endif
