

#ifndef AVUTIL_EXECUTOR_H
#define AVUTIL_EXECUTOR_H

typedef struct AVExecutor AVExecutor;
typedef struct AVTask AVTask;

struct AVTask {
    AVTask *next;
};

typedef struct AVTaskCallbacks {
    void *user_data;

    int local_context_size;

    
    int (*priority_higher)(const AVTask *a, const AVTask *b);

    
    int (*ready)(const AVTask *t, void *user_data);

    
    int (*run)(AVTask *t, void *local_context, void *user_data);
} AVTaskCallbacks;


AVExecutor* av_executor_alloc(const AVTaskCallbacks *callbacks, int thread_count);


void av_executor_free(AVExecutor **e);


void av_executor_execute(AVExecutor *e, AVTask *t);

#endif 
