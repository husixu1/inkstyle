#ifndef VISITORPATTERN_HPP
#define VISITORPATTERN_HPP

#include <functional>

template <typename Fn>
struct single_arg_function_traits;

template <typename R, typename C, typename Arg>
struct single_arg_function_traits<R (C::*)(Arg) const> {
    using decayed_arg_type = std::decay_t<Arg>;
};

template <typename... Ts>
struct Visitor;

template <typename T>
struct Visitor<T> {
    std::function<void(T &)> visitor;
    virtual void visit(T &visitable) final { visitor(visitable); };
    explicit Visitor(std::function<void(T &)> visitor) : visitor(visitor) {}
};

template <typename... Ts>
struct Visitor : Visitor<Ts>... {
    using Visitor<Ts>::visit...;
    Visitor(std::function<void(Ts &)>... visitors) : Visitor<Ts>(visitors)... {}
};

template <typename... Fns>
Visitor(Fns... visitors)
    -> Visitor<typename single_arg_function_traits<Fns>::decayed_arg_type...>;

template <typename VisitorType>
struct Visitable {
    virtual void accept(VisitorType &visitor) = 0;
    inline void accept(VisitorType &&visitor) { accept(visitor); }
};

#endif // VISITORPATTERN_HPP
