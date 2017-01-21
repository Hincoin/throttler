#include <numeric>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <cstdint>

struct flag
{
    std::uint64_t executions;
    std::uint64_t minute;
};

std::atomic<flag> atomic_state;
int arr[8] = {};


template<std::size_t N>
void execute_per_minute(void(* func)(int), int arg)
{
    flag current_state;
    
    for(;;)
    {
        current_state = atomic_state.load(std::memory_order_acquire);
        
        std::uint64_t executions = 0;
        auto clock = std::chrono::system_clock::now();
        std::uint64_t now = std::chrono::duration_cast<std::chrono::minutes>(clock.time_since_epoch()).count();
        
        
        if(now < current_state.minute)
            continue; // received outdated state
        
        if(now == current_state.minute)
            executions = current_state.executions + 1;
        
        if(executions >= N)
            return;
        
        flag new_state = {executions, now};
        
        if(atomic_state.compare_exchange_weak(current_state, new_state, std::memory_order_relaxed, std::memory_order_relaxed))
            break;
    }
    
    func(arg);
    
    std::atomic_thread_fence(std::memory_order_release);
}

void work(int tid)
{
    std::this_thread::sleep_for(std::chrono::microseconds(512));
    ++arr[tid];
}

int main()
{
    constexpr int N = 120;
    std::vector<std::thread> threads;
    
    for(int i = 0; i < 8; ++i)
    {
        threads.emplace_back([i, N]()
           {
        
               for(int j = 0; j < 1000; ++j)
               {
                   execute_per_minute<N>(work, i);
               }
               
               
           });
    }
    
    for(auto&& thread : threads)
        thread.join();
    
    std::cout << "Results: ";
    for(auto thread_value : arr)
        std::cout << thread_value << '\n';
    
    auto sum = std::accumulate(std::begin(arr), std::end(arr), 0);
    if(sum != N)
        std::cout << "BUG! Sum was " << sum << " expected " << N << '\n';
    else
        std::cout << "Sum correct!\n";

}
