
#include <cstddef>
#include <type_traits>
#include "Types.hpp"
namespace Allocated {

template<typename A,std::enable_if_t<types::traits<A>::defined> = true>
struct Stack {
    using type = A;
}; 

template<typename A,std::enable_if_t<types::traits<A>::defined> = true>
struct Static {
    using type = A;
    
    ~Static() = delete; // not stack allocated or implicit lifetime
};
template<typename A,std::enable_if_t<types::traits<A>::defined> = true>
struct GC {
    using type = A;
    ~GC() = delete; // not stack allocated or implicit lifetime
};
template<typename A,std::enable_if_t<types::traits<A>::defined> = true>
struct Malloc {
    using type = A;
    ~Malloc() = delete; // not stack allocated or implicit lifetime
   
};

}
