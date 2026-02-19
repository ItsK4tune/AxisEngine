#ifndef ENTT_CORE_ALGORITHM_HPP
#define ENTT_CORE_ALGORITHM_HPP

#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>
#include <vector>
#include "utility.hpp"

namespace entt {


struct std_sort {
    
    template<typename It, typename Compare = std::less<>, typename... Args>
    void operator()(It first, It last, Compare compare = Compare{}, Args &&...args) const {
        std::sort(std::forward<Args>(args)..., std::move(first), std::move(last), std::move(compare));
    }
};


struct insertion_sort {
    
    template<typename It, typename Compare = std::less<>>
    void operator()(It first, It last, Compare compare = Compare{}) const {
        if(first < last) {
            for(auto it = first + 1; it < last; ++it) {
                auto value = std::move(*it);
                auto pre = it;

                
                for(; pre > first && compare(value, *(pre - 1)); --pre) {
                    *pre = std::move(*(pre - 1));
                }
                

                *pre = std::move(value);
            }
        }
    }
};


template<std::size_t Bit, std::size_t N>
struct radix_sort {
    static_assert((N % Bit) == 0, "The maximum number of bits to sort must be a multiple of the number of bits processed per pass");

    
    template<typename It, typename Getter = identity>
    void operator()(It first, It last, Getter getter = Getter{}) const {
        if(first < last) {
            constexpr auto passes = N / Bit;

            using value_type = typename std::iterator_traits<It>::value_type;
            using difference_type = typename std::iterator_traits<It>::difference_type;
            std::vector<value_type> aux(static_cast<std::size_t>(std::distance(first, last)));

            auto part = [getter = std::move(getter)](auto from, auto to, auto out, auto start) {
                constexpr auto mask = (1 << Bit) - 1;
                constexpr auto buckets = 1 << Bit;

                
                std::size_t count[buckets]{};

                for(auto it = from; it != to; ++it) {
                    ++count[(getter(*it) >> start) & mask];
                }

                
                std::size_t index[buckets]{};

                for(std::size_t pos{}, end = buckets - 1u; pos < end; ++pos) {
                    index[pos + 1u] = index[pos] + count[pos];
                }

                for(auto it = from; it != to; ++it) {
                    const auto pos = index[(getter(*it) >> start) & mask]++;
                    out[static_cast<difference_type>(pos)] = std::move(*it);
                }
            };

            for(std::size_t pass = 0; pass < (passes & ~1u); pass += 2) {
                part(first, last, aux.begin(), pass * Bit);
                part(aux.begin(), aux.end(), first, (pass + 1) * Bit);
            }

            if constexpr(passes & 1) {
                part(first, last, aux.begin(), (passes - 1) * Bit);
                std::move(aux.begin(), aux.end(), first);
            }
        }
    }
};

} 

#endif
