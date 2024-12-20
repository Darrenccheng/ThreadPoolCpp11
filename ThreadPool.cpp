#include "ThreadPool.h"
#include <iostream>

ThreadPool::ThreadPool(int min, int max) : minThreads_(min), maxThreads_(max),
                                            curThreads_(max), idleThreads_(max), stop_(false) {
    cout << "max = " << max << endl;
    // 创建管理者线程
    manager_ = new thread(&ThreadPool::manager, this);
    // 创建工作线程
    for (int i = 0; i < min; ++i) {
        thread t(&ThreadPool::worker, this);
        workers_.insert(make_pair(t.get_id(), move(t)));
    }
}

ThreadPool::~ThreadPool() {
    // 让线程池关闭，之后线程都会推出程序
    stop_ = true;
    // 唤醒阻塞在条件变量处的线程
    condition_.notify_all();
    // 判断工作线程是否需要释放
    for (auto& it : workers_) {
        if (it.second.joinable()) {
            // 此处只是释放了资源，线程会在map退出时被析构
            // 输出要放到join之前
            cout << "线程" << it.second.get_id() << "将要退出了" << endl;
            it.second.join();
        }
    }
    // 释放工作者线程
    if (manager_->joinable()) {
        manager_->join();
    }
    delete manager_;
}

//// 添加任务到任务队列
//void ThreadPool::addTask(function<void(void)> task) {
//    {
//        lock_guard<mutex> locker(queueMutex_);
//        tasks_.push(task);
//    }
//    // 任务队列有任务了，唤醒一个阻塞的子线程
//    condition_.notify_one();
//}

void ThreadPool::manager() {
    while (!stop_) {
        // 每隔3秒钟检测一下是否需要删除或添加工作线程
        this_thread::sleep_for(chrono::microseconds (1));

        int idle = idleThreads_.load();
        int cur = curThreads_.load();

        // 判断是否需要杀死线程
        if (idle > cur / 2 && cur > minThreads_.load()) {
            // 每次杀死两个线程
            exitThreads_.store(2);
            condition_.notify_all();

            lock_guard<mutex> lockids(idsMutex_);
            for (auto id : ids_) {
                auto it = workers_.find(id);
                if (it != workers_.end()) {
                    cout << "线程被销毁了， ID：" << it->first << endl;
                    it->second.join();  // 释放线程资源
                    workers_.erase(it);

                }
            }
            ids_.clear();
        }
        else if (idle == 0 && cur < maxThreads_.load()) {
            // 添加线程
            thread t(&ThreadPool::worker, this);
            workers_.insert(make_pair(t.get_id(), move(t)));
            curThreads_++;
            idleThreads_++;
        }
    }
}

void ThreadPool::worker() {
    while (!stop_) {
        function<void(void)> task = nullptr;
        {
            unique_lock<mutex> locker(queueMutex_);
            while (tasks_.empty() && !stop_) {
                condition_.wait(locker);

                // 看是否需要自杀退出
                if (exitThreads_.load() > 0) {
                    exitThreads_--;
                    curThreads_--;
                    idleThreads_--;
                    cout << "线程退出了，ID： " << this_thread::get_id() << endl;
                    lock_guard<mutex> lockids(idsMutex_);
                    ids_.push_back(this_thread::get_id());
                    return;
                }
            }
            if (!tasks_.empty()) {
                cout << "取出了一个任务。。。" << endl;
                task = move(tasks_.front());    // 提高效率，避免拷贝
                tasks_.pop();
            }
        }

        if (task) {
            // 执行任务函数
            idleThreads_--;
            task();
            idleThreads_++;
        }
    }
}









