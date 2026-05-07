// First wave of functional tests using test_harness.h.
//
// These cover operators that are already registered by the C++ side
// at construction time — no sli-init.sli load is required. Each
// snippet is a small SLI source string; the harness evaluates it and
// asserts on the resulting operand stack.
//
// Adding a new test: pick (or create) a section header for the
// operator family, add a line, build, run. Failures point at the
// source line and print the snippet that broke.

#include "test_harness.h"

#include <iostream>

int main()
{
    sli3::SLIInterpreter i;

    // -------- literal pushes --------------------------------------
    EVAL_INT(i,    "42",          42);
    EVAL_INT(i,    "-7",          -7);
    EVAL_DOUBLE(i, "3.5",         3.5,  1e-12);
    EVAL_DOUBLE(i, "-2.25",      -2.25, 1e-12);
    EVAL_BOOL(i,   "true",        true);
    EVAL_BOOL(i,   "false",       false);
    EVAL_STRING(i, "(hello)",     "hello");

    // -------- integer arithmetic (typed leaves) -------------------
    EVAL_INT(i, "1 1 add_ii",     2);
    EVAL_INT(i, "10 3 sub_ii",    7);
    EVAL_INT(i, "6 7 mul_ii",     42);
    EVAL_INT(i, "20 4 div_ii",    5);

    // -------- double arithmetic -----------------------------------
    EVAL_DOUBLE(i, "1.5 2.5 add_dd", 4.0,  1e-12);
    EVAL_DOUBLE(i, "5.0 1.5 sub_dd", 3.5,  1e-12);
    EVAL_DOUBLE(i, "2.0 3.0 mul_dd", 6.0,  1e-12);
    EVAL_DOUBLE(i, "9.0 4.0 div_dd", 2.25, 1e-12);

    // -------- mixed int/double ------------------------------------
    EVAL_DOUBLE(i, "1 2.0 add_id",   3.0, 1e-12);
    EVAL_DOUBLE(i, "1.0 2 add_di",   3.0, 1e-12);

    // -------- comparisons -----------------------------------------
    EVAL_BOOL(i, "2 1 gt_ii",        true);
    EVAL_BOOL(i, "1 2 gt_ii",        false);
    EVAL_BOOL(i, "1 2 lt_ii",        true);
    EVAL_BOOL(i, "2 2 leq_ii",       true);
    EVAL_BOOL(i, "3 2 geq_ii",       true);
    EVAL_BOOL(i, "2.0 1.0 gt_dd",    true);

    // -------- string ordering (lt_ss / gt_ss) ---------------------
    EVAL_BOOL(i, "(abc) (abd) lt_ss",  true);
    EVAL_BOOL(i, "(abd) (abc) lt_ss",  false);
    EVAL_BOOL(i, "(abc) (abc) lt_ss",  false);  // equal → not less
    EVAL_BOOL(i, "(ab)  (abc) lt_ss",  true);   // prefix: shorter is less
    EVAL_BOOL(i, "(abc) (ab)  lt_ss",  false);
    EVAL_BOOL(i, "()    (a)   lt_ss",  true);   // empty < any non-empty

    EVAL_BOOL(i, "(abd) (abc) gt_ss",  true);
    EVAL_BOOL(i, "(abc) (abd) gt_ss",  false);
    EVAL_BOOL(i, "(abc) (abc) gt_ss",  false);
    EVAL_BOOL(i, "(abc) (ab)  gt_ss",  true);
    EVAL_BOOL(i, "(ab)  (abc) gt_ss",  false);
    EVAL_BOOL(i, "(a)   ()    gt_ss",  true);

    // -------- generic eq / neq ------------------------------------
    EVAL_BOOL(i, "1 1 eq",           true);
    EVAL_BOOL(i, "1 2 eq",           false);
    EVAL_BOOL(i, "1 2 neq",          true);
    EVAL_BOOL(i, "(abc) (abc) eq",   true);
    EVAL_BOOL(i, "(abc) (abd) eq",   false);

    // -------- stack operators -------------------------------------
    // Combine with arithmetic so each test reduces to a depth-1
    // value assertion. Depth-only checks cover the multi-item shapes.
    EVAL_INT(i,   "5 dup add_ii",         10);  // dup: 5 5 → add: 10
    EVAL_INT(i,   "1 2 exch sub_ii",       1);  // exch: 2 1 → 2-1
    EVAL_DEPTH(i, "1 2 dup",               3);
    EVAL_DEPTH(i, "1 2 exch",              2);
    EVAL_DEPTH(i, "1 2 3 pop pop",         1);
    EVAL_DEPTH(i, "1 2 3 pop pop pop",     0);

    // -------- name binding (def / lookup) -------------------------
    EVAL_INT(i, "/x 42 def x",       42);
    EVAL_INT(i, "/x 1 def /y 2 def x y add_ii", 3);

    // -------- conditionals ----------------------------------------
    EVAL_INT(i,   "true { 1 } if",    1);
    EVAL_DEPTH(i, "false { 1 } if",   0);
    EVAL_INT(i,   "true { 1 } { 2 } ifelse",  1);
    EVAL_INT(i,   "false { 1 } { 2 } ifelse", 2);

    // -------- for loop --------------------------------------------
    // for: start step stop proc for. Pushes each i in [start, start+step, ...]
    // up to and including stop (when step is positive).
    EVAL_INT(i,   "0 1 1 5 { add_ii } for",     15);  // sum 1..5
    EVAL_INT(i,   "0 0 2 10 { add_ii } for",    30);  // sum 0,2,4,6,8,10
    EVAL_DEPTH(i, "1 1 3 { } for",               3);  // 1 2 3 left on stack
    EVAL_INT(i,   "1 1 3 { } for add_ii add_ii", 6);  // 1+2+3

    // -------- forall_a / forall_s ---------------------------------
    // forall_a: array proc forall_a — execute proc once per element,
    // pushing the element onto the operand stack first.
    EVAL_INT(i,   "0 [1 2 3 4] { add_ii } forall_a",  10);
    EVAL_DEPTH(i, "[10 20 30] { } forall_a",           3);  // 10 20 30
    EVAL_INT(i,   "[10 20 30] { } forall_a add_ii add_ii", 60);
    EVAL_DEPTH(i, "[] { 99 } forall_a",                0);  // empty: never runs

    // forall_s: walks a string by character code (int per char).
    EVAL_INT(i,   "0 (abc) { add_ii } forall_s",  97+98+99);
    EVAL_DEPTH(i, "() { 99 } forall_s",            0);

    // -------- forallindexed_a / forallindexed_s -------------------
    // forallindexed: pushes value then index per iteration.
    // [10 20 30] {} forallindexed_a -> 10 0 20 1 30 2 (n*2 items)
    EVAL_DEPTH(i, "[10 20 30] { } forallindexed_a",   6);
    // Sum value+index over the whole array:
    EVAL_INT(i,   "0 [10 20 30] { add_ii add_ii } forallindexed_a",
                   10+0 + 20+1 + 30+2);  // 63

    // String version: each char as int + index
    EVAL_INT(i,   "0 (abc) { add_ii add_ii } forallindexed_s",
                   97+0 + 98+1 + 99+2);  // 297

    // -------- math doubles: trig ----------------------------------
    // Note: pi ≈ 3.141592653589793, e ≈ 2.718281828459045
    EVAL_DOUBLE(i, "0.0 sin_d",                 0.0,  1e-12);
    EVAL_DOUBLE(i, "1.5707963267948966 sin_d",  1.0,  1e-12);  // sin(pi/2)
    EVAL_DOUBLE(i, "3.141592653589793 sin_d",   0.0,  1e-12);
    EVAL_DOUBLE(i, "0.0 cos_d",                 1.0,  1e-12);
    EVAL_DOUBLE(i, "3.141592653589793 cos_d",  -1.0,  1e-12);
    EVAL_DOUBLE(i, "1.0 asin_d",  1.5707963267948966, 1e-12);  // pi/2
    EVAL_DOUBLE(i, "0.0 asin_d",                0.0,  1e-12);
    EVAL_DOUBLE(i, "1.0 acos_d",                0.0,  1e-12);
    EVAL_DOUBLE(i, "0.0 acos_d", 1.5707963267948966,  1e-12);

    // -------- math doubles: exp / log -----------------------------
    EVAL_DOUBLE(i, "0.0 exp_d",                 1.0,  1e-12);
    EVAL_DOUBLE(i, "1.0 exp_d", 2.718281828459045,    1e-12);
    EVAL_DOUBLE(i, "1.0 ln_d",                  0.0,  1e-12);
    EVAL_DOUBLE(i, "2.718281828459045 ln_d",    1.0,  1e-12);
    EVAL_DOUBLE(i, "1.0 log_d",                 0.0,  1e-12);  // log10
    EVAL_DOUBLE(i, "10.0 log_d",                1.0,  1e-12);
    EVAL_DOUBLE(i, "100.0 log_d",               2.0,  1e-12);

    // -------- math doubles: powers / roots ------------------------
    EVAL_DOUBLE(i, "2.0 sqr_d",                 4.0,  1e-12);
    EVAL_DOUBLE(i, "-3.0 sqr_d",                9.0,  1e-12);
    EVAL_DOUBLE(i, "4.0 sqrt_d",                2.0,  1e-12);
    EVAL_DOUBLE(i, "2.0 sqrt_d", 1.4142135623730951,  1e-12);
    EVAL_DOUBLE(i, "2.0 3.0 pow_dd",            8.0,  1e-12);
    EVAL_DOUBLE(i, "9.0 0.5 pow_dd",            3.0,  1e-12);
    EVAL_DOUBLE(i, "2.0 10  pow_di",            1024.0, 1e-9);
    EVAL_DOUBLE(i, "2.0 inv",                   0.5,  1e-12);
    EVAL_DOUBLE(i, "4.0 inv",                   0.25, 1e-12);

    // -------- math doubles: sign / abs / neg ----------------------
    EVAL_DOUBLE(i, "-3.5 abs_d",                3.5,  1e-12);
    EVAL_DOUBLE(i, "3.5  abs_d",                3.5,  1e-12);
    EVAL_INT(i,    "-7 abs_i",                  7);
    EVAL_INT(i,    "5  abs_i",                  5);
    EVAL_DOUBLE(i, "3.5  neg_d",               -3.5,  1e-12);
    EVAL_DOUBLE(i, "-3.5 neg_d",                3.5,  1e-12);
    EVAL_INT(i,    "5  neg_i",                 -5);
    EVAL_INT(i,    "-7 neg_i",                  7);

    // -------- math doubles: rounding ------------------------------
    // round_d / floor_d / ceil_d return doubles (not ints).
    EVAL_DOUBLE(i, "0.7 round_d",               1.0,  1e-12);
    EVAL_DOUBLE(i, "0.4 round_d",               0.0,  1e-12);
    EVAL_DOUBLE(i, "1.5 round_d",               2.0,  1e-12);
    EVAL_DOUBLE(i, "1.7 floor_d",               1.0,  1e-12);
    EVAL_DOUBLE(i, "-1.3 floor_d",             -2.0,  1e-12);
    EVAL_DOUBLE(i, "1.3 ceil_d",                2.0,  1e-12);
    EVAL_DOUBLE(i, "-1.7 ceil_d",              -1.0,  1e-12);

    // int_d truncates toward zero, returns integer.
    EVAL_INT(i,    "3.7 int_d",                 3);
    EVAL_INT(i,    "-3.7 int_d",               -3);
    EVAL_INT(i,    "5.0 int_d",                 5);

    // -------- math doubles: min / max -----------------------------
    EVAL_INT(i,    "3 5 max_i_i",               5);
    EVAL_INT(i,    "5 3 max_i_i",               5);
    EVAL_INT(i,    "3 5 min_i_i",               3);
    EVAL_DOUBLE(i, "3.5 5.5 max_d_d",           5.5,  1e-12);
    EVAL_DOUBLE(i, "3.5 5.5 min_d_d",           3.5,  1e-12);

    // -------- math doubles: modf / frexp (multi-output) -----------
    // 3.7 modf_d -> 0.7 3.0  (frac, int-part-as-double)
    EVAL_DEPTH(i,  "3.7 modf_d", 2);
    EVAL_DOUBLE(i, "3.7 modf_d pop",           0.7,  1e-12);   // frac
    EVAL_DOUBLE(i, "3.7 modf_d exch pop",      3.0,  1e-12);   // int part

    // 12.0 frexp_d -> 0.75 4  (mantissa, exponent_int)
    EVAL_DEPTH(i,  "12.0 frexp_d", 2);
    EVAL_INT(i,    "12.0 frexp_d exch pop",     4);            // exponent
    EVAL_DOUBLE(i, "12.0 frexp_d pop",         0.75,  1e-12);  // mantissa

    // -------- getinterval_a / getinterval_s -----------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc:
    //   obj index count getinterval -> subobj
    // Strict bounds: index in [0, size), count >= 0, index+count <= size.
    // Result is a fresh container (copy of [index, index+count)).

    // Array success cases — check value via length / get / arrayload.
    EVAL_INT(i,    "[10 20 30 40 50] 0 3 getinterval_a length_a",    3);
    EVAL_INT(i,    "[10 20 30 40 50] 0 3 getinterval_a 0 get_a", 10);
    EVAL_INT(i,    "[10 20 30 40 50] 0 3 getinterval_a 2 get_a", 30);
    EVAL_INT(i,    "[10 20 30 40 50] 2 3 getinterval_a 0 get_a", 30);
    EVAL_INT(i,    "[10 20 30 40 50] 2 3 getinterval_a 2 get_a", 50);
    EVAL_INT(i,    "[10 20 30 40 50] 4 1 getinterval_a 0 get_a", 50);
    EVAL_INT(i,    "[10 20 30] 0 0 getinterval_a length_a",          0);  // empty
    EVAL_INT(i,    "[10 20 30] 0 3 getinterval_a length_a",          3);  // full
    EVAL_INT(i,    "[42] 0 1 getinterval_a 0 get_a",            42);  // size-1

    // String success cases — confirm length and a probe character.
    EVAL_INT(i,    "(hello) 0 5 getinterval_s length_s",             5);
    EVAL_INT(i,    "(hello) 1 3 getinterval_s length_s",             3);
    EVAL_INT(i,    "(hello) 1 3 getinterval_s 0 get_s",            'e');
    EVAL_INT(i,    "(hello) 1 3 getinterval_s 2 get_s",            'l');
    EVAL_INT(i,    "(hello) 4 1 getinterval_s 0 get_s",            'o');
    EVAL_INT(i,    "(hello) 0 0 getinterval_s length_s",             0);  // empty

    // Source must be popped — only the result remains.
    EVAL_DEPTH(i,  "[10 20 30] 0 2 getinterval_a",                 1);
    EVAL_DEPTH(i,  "(hello) 1 3 getinterval_s",                    1);

    std::cout << "test_sli_eval: all assertions passed\n";
    return 0;
}
