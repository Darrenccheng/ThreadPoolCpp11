#include <iostream>
#include "ThreadPool.h"

int func(int x, int y) {
    int res = x + y;
    // cout << "z = " << res << endl;
    this_thread::sleep_for(chrono::seconds(3));
    return res;
}

int main() {
    ThreadPool pool(4);
    vector<future<int>> results;

    for (int i = 0; i < 10; i++) {
        // auto task = bind(func, i, i * 2);
        results.emplace_back(pool.addTask(func, i, i * 2));
    }

    for (auto&& res : results) {
        cout << "线程返回结果： " << res.get() << endl;
    }

    return 0;
}
