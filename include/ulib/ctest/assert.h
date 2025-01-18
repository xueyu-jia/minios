#pragma once

#include <stdbool.h>
#include <macro_helper.h>
#include <compiler.h>
#include <string.h> // IWYU pragma: export

void CTEST_notify_assertion_failure(const char *file, int line, const char *fmt, ...);

#define CTEST_PRED_EVALUATE_(op, val1, val2) MH_CONCAT(CTEST_PRED_EVALUATE_, op)(lhs, rhs)

#define CTEST_PRED_OPERAND_TYPE_(type) MH_CONCAT(CTEST_PRED_OPERAND_TYPE_, type)
#define CTEST_PRED_OPERAND_TYPE_int "%d"
#define CTEST_PRED_OPERAND_TYPE_str "\"%s\""
#define CTEST_PRED_OPERAND_TYPE_f32 "%f"
#define CTEST_PRED_OPERAND_TYPE_f64 "%lf"

#define CTEST_PRED_ENDAS_(type) MH_CONCAT(CTEST_PRED_ENDAS_, type)
#define CTEST_PRED_ENDAS_EXPECT
#define CTEST_PRED_ENDAS_ASSERT return

#define CTEST_PRED_FAILURE_MSG_FMT(op, val_type) \
    "Expected operation %s holds true for these operands:\n" \
    "  LHS: %s\n"                                            \
    "    Which is: " CTEST_PRED_OPERAND_TYPE_(val_type) "\n" \
    "  RHS: %s\n"                                            \
    "    Which is: " CTEST_PRED_OPERAND_TYPE_(val_type) "\n"

#define CTEST_PRED_(type, op, val_type, val1, val2)                                                \
    do {                                                                                           \
        const auto lhs = (val1);                                                                   \
        const auto rhs = (val2);                                                                   \
        if (CTEST_PRED_EVALUATE_(op, lhs, rhs)) { break; }                                         \
        CTEST_notify_assertion_failure(__FILE__, __LINE__,                                         \
                                       CTEST_PRED_FAILURE_MSG_FMT(op, val_type), MH_STRINGIFY(op), \
                                       MH_STRINGIFY(val1), lhs, MH_STRINGIFY(val2), rhs);          \
        CTEST_PRED_ENDAS_(type);                                                                   \
    } while (0)

#define CTEST_PRED_EVALUATE_EQ(lhs, rhs) ((lhs) == (rhs))
#define CTEST_PRED_EVALUATE_NE(lhs, rhs) ((lhs) != (rhs))
#define CTEST_PRED_EVALUATE_LE(lhs, rhs) ((lhs) <= (rhs))
#define CTEST_PRED_EVALUATE_LT(lhs, rhs) ((lhs) < (rhs))
#define CTEST_PRED_EVALUATE_GE(lhs, rhs) ((lhs) >= (rhs))
#define CTEST_PRED_EVALUATE_GT(lhs, rhs) ((lhs) > (rhs))
#define CTEST_PRED_EVALUATE_STREQ(lhs, rhs) (strcmp(lhs, rhs) == 0)
#define CTEST_PRED_EVALUATE_STRNE(lhs, rhs) (strcmp(lhs, rhs) != 0)
#define CTEST_PRED_EVALUATE_STRCASEEQ(lhs, rhs) (stricmp(lhs, rhs) == 0)
#define CTEST_PRED_EVALUATE_STRCASENE(lhs, rhs) (stricmp(lhs, rhs) != 0)
#define CTEST_PRED_EVALUATE_FLOAT_EQ(lhs, rhs) (((float)(lhs)) == ((float)(rhs)))
#define CTEST_PRED_EVALUATE_DOUBLE_EQ(lhs, rhs) (((double)(lhs)) == ((double)(rhs)))

#define EXPECT_EQ(val1, val2) CTEST_PRED_(EXPECT, EQ, int, val1, val2)
#define EXPECT_NE(val1, val2) CTEST_PRED_(EXPECT, NE, int, val1, val2)
#define EXPECT_LE(val1, val2) CTEST_PRED_(EXPECT, LE, int, val1, val2)
#define EXPECT_LT(val1, val2) CTEST_PRED_(EXPECT, LT, int, val1, val2)
#define EXPECT_GE(val1, val2) CTEST_PRED_(EXPECT, GE, int, val1, val2)
#define EXPECT_GT(val1, val2) CTEST_PRED_(EXPECT, GT, int, val1, val2)

#define EXPECT_TRUE(val) EXPECT_EQ(!!(val), true)
#define EXPECT_FALSE(val) EXPECT_EQ(!!(val), false)

#define EXPECT_STREQ(s1, s2) CTEST_PRED_(EXPECT, STREQ, str, s1, s2)
#define EXPECT_STRNE(s1, s2) CTEST_PRED_(EXPECT, STRNE, str, s1, s2)
#define EXPECT_STRCASEEQ(s1, s2) CTEST_PRED_(EXPECT, STRCASEEQ, str, s1, s2)
#define EXPECT_STRCASENE(s1, s2) CTEST_PRED_(EXPECT, STRCASENE, str, s1, s2)

#define EXPECT_FLOAT_EQ(val1, val2) CTEST_PRED_(EXPECT, FLOAT_EQ, f32, val1, val2)
#define EXPECT_DOUBLE_EQ(val1, val2) CTEST_PRED_(EXPECT, DOUBLE_EQ, f64, val1, val2)

#define ASSERT_EQ(val1, val2) CTEST_PRED_(ASSERT, EQ, int, val1, val2)
#define ASSERT_NE(val1, val2) CTEST_PRED_(ASSERT, NE, int, val1, val2)
#define ASSERT_LE(val1, val2) CTEST_PRED_(ASSERT, LE, int, val1, val2)
#define ASSERT_LT(val1, val2) CTEST_PRED_(ASSERT, LT, int, val1, val2)
#define ASSERT_GE(val1, val2) CTEST_PRED_(ASSERT, GE, int, val1, val2)
#define ASSERT_GT(val1, val2) CTEST_PRED_(ASSERT, GT, int, val1, val2)

#define ASSERT_TRUE(val) ASSERT_EQ(val, true)
#define ASSERT_FALSE(val) ASSERT_EQ(val, false)

#define ASSERT_STREQ(s1, s2) CTEST_PRED_(ASSERT, STREQ, str, s1, s2)
#define ASSERT_STRNE(s1, s2) CTEST_PRED_(ASSERT, STRNE, str, s1, s2)
#define ASSERT_STRCASEEQ(s1, s2) CTEST_PRED_(ASSERT, STRCASEEQ, str, s1, s2)
#define ASSERT_STRCASENE(s1, s2) CTEST_PRED_(ASSERT, STRCASENE, str, s1, s2)

#define ASSERT_FLOAT_EQ(val1, val2) CTEST_PRED_(ASSERT, FLOAT_EQ, f32, val1, val2)
#define ASSERT_DOUBLE_EQ(val1, val2) CTEST_PRED_(ASSERT, DOUBLE_EQ, f64, val1, val2)
