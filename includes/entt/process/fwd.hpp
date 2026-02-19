#ifndef ENTT_PROCESS_FWD_HPP
#define ENTT_PROCESS_FWD_HPP

#include <cstdint>
#include <memory>

namespace entt {

template<typename, typename = std::allocator<void>>
class basic_process;


using process = basic_process<std::uint32_t>;

template<typename, typename = std::allocator<void>>
class basic_scheduler;


using scheduler = basic_scheduler<std::uint32_t>;

} 

#endif
