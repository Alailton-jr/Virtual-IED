#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <functional>
#include <vector>
#include <memory>

/**
 * @brief Task struct encapsulates a function and its arguments.
 * 
 * @tparam FuncType The type of the function to be executed.
 * @param func The function to execute.
 * @param args The arguments to pass to the function.
 */
template <typename FuncType>
struct Task {
    std::function<FuncType> func; // Function to execute.
    using ArgType = typename std::function<FuncType>::argument_type; // Type of function argument.
    std::shared_ptr<ArgType> args; // Shared pointer to function arguments.

    Task() = default; // Default constructor.
    Task(std::function<FuncType> f, std::shared_ptr<ArgType> a) : func(f), args(a) {} ///< Constructor with function and arguments.
};

/**
 * @brief ThreadPool class manages a pool of threads executing tasks from a queue.
 * 
 * @tparam FuncType The type of function managed by the thread pool.
 */
template <typename FuncType>
class ThreadPool {
private:
    // Threads and synchronization variables
    std::vector<pthread_t> threads; // Threads in the pool.
    bool stop, running; // Flags to control thread execution.
    pthread_cond_t not_empty, not_full; // Condition variables.
    std::vector<Task<FuncType>> taskQueue; // Task queue.
    pthread_mutex_t mutex; // Mutex to protect task queue.
    size_t num_tasks; // Maximum number of tasks in the queue.
    int32_t front, rear, count; // Indices and count of tasks in the queue.

public:
    /**
     * @brief Constructor initializes threads and task queue.
     * 
     * @param no_threads Number of threads in the thread pool.
     * @param no_task Maximum number of tasks in the queue.
     * @param priority Priority of threads (SCHED_FIFO).
    */
    ThreadPool(int32_t no_threads, int32_t no_task, int32_t priority) {
        stop = false;
        running = true;

        // Task Queue Initialization
        taskQueue.resize(no_task);
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init(&not_empty, nullptr);
        pthread_cond_init(&not_full, nullptr);
        num_tasks = no_task;
        front = count = 0;
        rear = -1;

        // Threads Initialization
        threads.resize(no_threads);
        struct sched_param schedParam{
            .sched_priority = priority
        };
        for (int32_t i = 0; i < no_threads; ++i) {
            pthread_create(&threads[i], nullptr, &ThreadPool<FuncType>::worker, this);
            pthread_setschedparam(threads[i], SCHED_FIFO, &schedParam);
        }
    }

    // Destructor stops threads and cleans up resources
    ~ThreadPool() {
        stop = true;

        // Signal threads to wake up and terminate
        pthread_cond_broadcast(&not_empty);
        for (pthread_t& thread : threads) {
            pthread_join(thread, nullptr);
        }

        // Cleanup synchronization resources
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&not_empty);
        pthread_cond_destroy(&not_full);
    }

    /**
     * @brief Submit a task to the thread pool for execution.
     * 
     * @param func Function to execute.
     * @param arg Shared pointer to function arguments.
     */
    void submit(std::function<FuncType> func, std::shared_ptr<typename std::function<FuncType>::argument_type> arg) {
        if (!stop) {
            push(Task<FuncType>(func, arg));
        }
    }

private:
    // Push a task onto the task queue
    void push(Task<FuncType> task) {
        pthread_mutex_lock(&mutex);
        while (count >= num_tasks) { // Wait for the queue to have space
            pthread_cond_wait(&not_full, &mutex);
        }
        rear++;
        if (rear == num_tasks) rear = 0;
        taskQueue[rear] = std::move(task); // Move the task into the queue
        count++; 
        pthread_cond_signal(&not_empty); // Signal that the queue is not empty
        pthread_mutex_unlock(&mutex); 
    }

    // Pop a task from the task queue
    Task<FuncType> pop(bool& stop) {
        pthread_mutex_lock(&mutex);
        while (count <= 0 && !stop) { // Wait for the queue to have tasks
            pthread_cond_wait(&not_empty, &mutex);
        }
        if (stop){ // If the thread pool is stopping, return an empty task
            pthread_mutex_unlock(&mutex);
            return Task<FuncType>(nullptr, nullptr);
        }
        
        Task task = taskQueue[front]; // Get the task from the front of the queue
        front++;
        if (front == num_tasks) front = 0;
        count--;

        pthread_cond_signal(&not_full); // Signal that the queue is not full
        pthread_mutex_unlock(&mutex);
        return task;
    }

    // Worker function for threads in the pool
    static void* worker(void* arg) {
        auto* pool = static_cast<ThreadPool*>(arg); // Get the thread pool from the argument
        while(!pool->stop) {
            Task<FuncType> task = pool->pop(pool->stop); // Get a task from the queue
            if (task.func == nullptr) break; // If the task is empty, stop the thread
            try {
                task.func(task.args.get()[0]); // Execute the task
            } catch (const std::exception& e) {
                std::cerr << "Exception in thread: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in thread" << std::endl;
            }
        }
        pool->running = false;
        return nullptr;
    }
};

#endif // THREAD_POOL_HPP
