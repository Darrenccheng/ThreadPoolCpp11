#pragma once
#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <map>
#include <future>
#include <memory>

using namespace std;

/**
* 1. 管理者线程 -> 子线程，1个
    - 控制工作线程的数量，创建或销毁工作者线程
2. 若干工作线程 -> 子线程，n个
   ```
   - 从任务队列中取出任务，并处理
   - 任务队列为空，被阻塞（使用条件变量）
   - 线程同步（互斥锁）
   - 当前线程数量、空闲线程数量
   - 最大最小线程数量（不确定最大最小值是否作为多线程共享资源被读写，就定义为原子变量，保证线程安全）
   ```
3. 任务队列 -> queue
   - 互斥锁
   - 条件变量（阻塞消费者线程）
4. 线程池开关 -> bool
*/

class ThreadPool {
public:
    ThreadPool(int min = 4, int max = 5);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto addTask(F&& f, Args&&... args) -> future<typename result_of<F(Args...)>::type> {
        // 1.packaged_task
        using returnType = typename result_of<F(Args...)>::type;
        auto returnTask = make_shared<packaged_task<returnType()>>(
                // 不知道传递的为右值还是左值引用，所以需要使用完美转发
                bind(forward<F>(f), forward<Args>(args)...)
                );

        // 2.获取future
        future<returnType> res = returnTask->get_future();
        // 3.放入到任务队列中给线程执行
        {
            lock_guard<mutex> locker(queueMutex_);
            tasks_.emplace([returnTask](){
                (*returnTask)();
            });
        }
        condition_.notify_one();

        return res;
    }

private:
    void manager(); // 管理者任务函数
    void worker();  // 工作者任务函数
private:
    thread* manager_;                   // 管理者线程
    vector<thread::id> ids_;            // 需要退出的线程id
    map<thread::id, thread> workers_;   // 工作线程
    atomic<int> curThreads_;            // 当前线程数量
    atomic<int> maxThreads_;            // 最大线程数量
    atomic<int> minThreads_;            // 最小线程数量
    atomic<int> idleThreads_;           // 空闲线程数量
    atomic<int> exitThreads_;           // 需要杀死的工作线程数量
    queue<function<void(void)>> tasks_; // 任务队列，若任务函数有参数，就使用bind绑定后传入
    mutex queueMutex_;                  // 任务队列的锁
    mutex idsMutex_;                    // ids的锁
    condition_variable condition_;      // 条件变量，阻塞消费者线程
    bool stop_;                         // 线程池开关
};
















