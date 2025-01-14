#pragma once

#define MH_COMMA ,

#define MH_EXPAND(...) __VA_ARGS__
#define MH_PAIR(x, y) x y

#define MH_INVOKE(fn, ...) MH_EXPAND(fn(__VA_ARGS__))

#define MH_CDR(first, ...) __VA_ARGS__
#define MH_CAR(first, ...) first

#define MH_STRINGIFY_IMPL(x) #x
#define MH_STRINGIFY_WRAPPER(x) MH_STRINGIFY_IMPL(x)
#define MH_STRINGIFY(x) MH_STRINGIFY_WRAPPER(x)

#define MH_CONCAT_IMPL(x, y) x##y
#define MH_CONCAT_WRAPPER(x, y) MH_CONCAT_IMPL(x, y)
#define MH_CONCAT(x, y) MH_CONCAT_WRAPPER(x, y)

#ifdef _MSC_VER
#define MH_NARG_GET(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, \
                    ...)                                                                          \
    N

#define MH_NARG_IMPL(...) \
    MH_EXPAND(MH_NARG_GET(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define MH_NARG(...) MH_PAIR(MH_NARG_IMPL, (, __VA_ARGS__))
#else
#define MH_NARG_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, \
                     ...)                                                                          \
    N

#define MH_NARG(...) \
    MH_EXPAND(       \
        MH_NARG_IMPL(0, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#endif

#define MH_FOREACH_IMPL0(pred, ...)
#define MH_FOREACH_IMPL1(pred, x, ...) pred(x)
#define MH_FOREACH_IMPL2(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL1(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL3(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL2(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL4(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL3(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL5(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL4(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL6(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL5(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL7(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL6(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL8(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL7(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL9(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL8(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL10(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL9(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL11(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL10(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL12(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL11(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL13(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL12(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL14(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL13(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL15(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL14(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL16(pred, x, ...) pred(x) MH_EXPAND(MH_FOREACH_IMPL15(pred, __VA_ARGS__))
#define MH_FOREACH_IMPL(N, pred, ...) MH_EXPAND(MH_CONCAT(MH_FOREACH_IMPL, N)(pred, __VA_ARGS__))
#define MH_FOREACH(pred, ...) \
    MH_EXPAND(MH_FOREACH_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), pred, __VA_ARGS__))

#define MH_FOREACH_PRED2_IMPL0(pred, t, ...)
#define MH_FOREACH_PRED2_IMPL1(pred, t, x, ...) pred(t, x)
#define MH_FOREACH_PRED2_IMPL2(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL1(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL3(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL2(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL4(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL3(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL5(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL4(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL6(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL5(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL7(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL6(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL8(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL7(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL9(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL8(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL10(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL9(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL11(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL10(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL12(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL11(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL13(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL12(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL14(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL13(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL15(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL14(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL16(pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_IMPL15(pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_IMPL(N, pred, t, ...) \
    MH_CONCAT(MH_FOREACH_PRED2_IMPL, N)(pred, t, __VA_ARGS__)
#define MH_FOREACH_PRED2(pred, t, ...) \
    MH_FOREACH_PRED2_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), pred, t, __VA_ARGS__)

#define MH_FOREACH_PRED2_ITER_IMPL0(iter, pred, t, ...)
#define MH_FOREACH_PRED2_ITER_IMPL1(iter, pred, t, x, ...) pred(t, x)
#define MH_FOREACH_PRED2_ITER_IMPL2(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL1(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL3(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL2(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL4(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL3(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL5(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL4(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL6(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL5(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL7(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL6(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL8(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL7(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL9(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL8(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL10(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL9(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL11(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL10(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL12(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL11(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL13(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL12(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL14(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL13(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL15(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL14(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL16(iter, pred, t, x, ...) \
    pred(t, x) MH_EXPAND(MH_FOREACH_PRED2_ITER_IMPL15(iter, pred, iter(t), __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER_IMPL(N, iter, pred, t, ...) \
    MH_EXPAND(MH_CONCAT(MH_FOREACH_PRED2_ITER_IMPL, N)(iter, pred, t, __VA_ARGS__))
#define MH_FOREACH_PRED2_ITER(iter, pred, t, ...) \
    MH_EXPAND(                                    \
        MH_FOREACH_PRED2_ITER_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), iter, pred, t, __VA_ARGS__))

#define MH_NEXT_INT_IMPL0 1
#define MH_NEXT_INT_IMPL1 2
#define MH_NEXT_INT_IMPL2 3
#define MH_NEXT_INT_IMPL3 4
#define MH_NEXT_INT_IMPL4 5
#define MH_NEXT_INT_IMPL5 6
#define MH_NEXT_INT_IMPL6 7
#define MH_NEXT_INT_IMPL7 8
#define MH_NEXT_INT_IMPL8 9
#define MH_NEXT_INT_IMPL9 10
#define MH_NEXT_INT_IMPL10 11
#define MH_NEXT_INT_IMPL11 12
#define MH_NEXT_INT_IMPL12 13
#define MH_NEXT_INT_IMPL13 14
#define MH_NEXT_INT_IMPL14 15
#define MH_NEXT_INT_IMPL15 16
#define MH_NEXT_INT(x) MH_CONCAT(MH_NEXT_INT_IMPL, x)

#define MH_ARG_WRAPPER_IMPL0(...)
#define MH_ARG_WRAPPER_IMPL1(arg, ...) arg
#define MH_ARG_WRAPPER_IMPL2(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL1(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL3(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL2(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL4(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL3(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL5(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL4(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL6(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL5(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL7(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL6(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL8(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL7(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL9(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL8(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL10(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL9(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL11(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL10(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL12(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL11(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL13(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL12(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL14(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL13(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL15(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL14(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL16(arg, ...) arg MH_COMMA MH_EXPAND(MH_ARG_WRAPPER_IMPL15(__VA_ARGS__))
#define MH_ARG_WRAPPER_IMPL(N, ...) MH_CONCAT(MH_ARG_WRAPPER_IMPL, N)(__VA_ARGS__)
#define MH_ARG_WRAPPER(...) \
    MH_EXPAND(MH_ARG_WRAPPER_IMPL(MH_EXPAND(MH_NARG(__VA_ARGS__)), __VA_ARGS__))
