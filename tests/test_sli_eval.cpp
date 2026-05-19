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

    // -------- join_a / join_p ------------------------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc:
    //   a1 a2 join_a -> a1 ++ a2   (fresh array; same for join_p)

    EVAL_INT(i,    "[1 2] [3 4] join_a length_a",                  4);
    EVAL_INT(i,    "[1 2] [3 4] join_a 0 get_a",                   1);
    EVAL_INT(i,    "[1 2] [3 4] join_a 3 get_a",                   4);
    EVAL_INT(i,    "[10 20 30] [40] join_a length_a",              4);
    EVAL_INT(i,    "[10 20 30] [40] join_a 3 get_a",              40);
    EVAL_INT(i,    "[] [1 2] join_a length_a",                     2);
    EVAL_INT(i,    "[1 2] [] join_a length_a",                     2);
    EVAL_INT(i,    "[] [] join_a length_a",                        0);
    EVAL_DEPTH(i,  "[1 2] [3 4] join_a",                           1);

    // join_p builds a procedure-typed result; length_p exposes its size.
    EVAL_INT(i,    "{1 2} {3 4} join_p length_p",                  4);
    EVAL_INT(i,    "{1 2 3} {} join_p length_p",                   3);
    EVAL_INT(i,    "{} {} join_p length_p",                        0);
    EVAL_DEPTH(i,  "{1 2} {3 4} join_p",                           1);

    // -------- insert_a / insert_s --------------------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc:
    //   c1 idx c2 insert_<a|s> -> c1'   (c2 inserted at idx; tail shifts right)
    // Strict validation: idx in [0, size); idx==size is RangeCheck.

    // Doc example: [20 21 22 24 25 26] 3 [23] insert -> [20 21 22 23 24 25 26]
    EVAL_INT(i,    "[20 21 22 24 25 26] 3 [23] insert_a length_a", 7);
    EVAL_INT(i,    "[20 21 22 24 25 26] 3 [23] insert_a 0 get_a", 20);
    EVAL_INT(i,    "[20 21 22 24 25 26] 3 [23] insert_a 3 get_a", 23);
    EVAL_INT(i,    "[20 21 22 24 25 26] 3 [23] insert_a 4 get_a", 24);
    EVAL_INT(i,    "[20 21 22 24 25 26] 3 [23] insert_a 6 get_a", 26);

    // Insert multi-element at front (prepend equivalent).
    EVAL_INT(i,    "[1 2 3] 0 [99 100] insert_a length_a",         5);
    EVAL_INT(i,    "[1 2 3] 0 [99 100] insert_a 0 get_a",         99);
    EVAL_INT(i,    "[1 2 3] 0 [99 100] insert_a 1 get_a",        100);
    EVAL_INT(i,    "[1 2 3] 0 [99 100] insert_a 2 get_a",          1);

    // Empty insert leaves source unchanged.
    EVAL_INT(i,    "[1 2 3] 0 [] insert_a length_a",               3);
    EVAL_INT(i,    "[1 2 3] 0 [] insert_a 0 get_a",                1);

    EVAL_DEPTH(i,  "[1 2] 0 [9] insert_a",                         1);

    // Doc example: (spikesimulation) 5 (train) insert -> (spiketrainsimulation)
    EVAL_INT(i,    "(spikesimulation) 5 (train) insert_s length_s", 20);
    EVAL_INT(i,    "(spikesimulation) 5 (train) insert_s 5 get_s",  't');
    EVAL_INT(i,    "(spikesimulation) 5 (train) insert_s 9 get_s",  'n');
    EVAL_INT(i,    "(spikesimulation) 5 (train) insert_s 10 get_s", 's');  // shifted

    EVAL_INT(i,    "(abc) 0 (XYZ) insert_s length_s",               6);
    EVAL_INT(i,    "(abc) 0 (XYZ) insert_s 0 get_s",                'X');
    EVAL_INT(i,    "(abc) 0 (XYZ) insert_s 3 get_s",                'a');

    EVAL_INT(i,    "(hello) 0 () insert_s length_s",                5);  // empty insert

    EVAL_DEPTH(i,  "(abc) 0 (X) insert_s",                          1);

    // -------- replace_a / replace_s ------------------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc:
    //   c1 idx n c2 replace_<a|s> -> c1'
    // Replaces the [idx, idx+n) section of c1 with c2; n+idx > size
    // silently clamps to remaining length.
    //   idx < 0 || idx >= size -> RangeCheckError
    //   n < 0                  -> PositiveIntegerExpectedError

    // Doc example: [1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace
    //              -> [1 2 99 99 99 99 6 7]
    EVAL_INT(i,    "[1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace_a length_a", 8);
    EVAL_INT(i,    "[1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace_a 0 get_a", 1);
    EVAL_INT(i,    "[1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace_a 1 get_a", 2);
    EVAL_INT(i,    "[1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace_a 2 get_a", 99);
    EVAL_INT(i,    "[1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace_a 5 get_a", 99);
    EVAL_INT(i,    "[1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace_a 6 get_a", 6);
    EVAL_INT(i,    "[1 2 3 4 5 6 7] 2 3 [99 99 99 99] replace_a 7 get_a", 7);

    // Same-length replace.
    EVAL_INT(i,    "[1 2 3] 0 3 [9 8 7] replace_a 0 get_a",         9);
    EVAL_INT(i,    "[1 2 3] 0 3 [9 8 7] replace_a 2 get_a",         7);

    // Replace with empty array (effectively erase a slice).
    EVAL_INT(i,    "[1 2 3 4 5] 1 2 [] replace_a length_a",         3);
    EVAL_INT(i,    "[1 2 3 4 5] 1 2 [] replace_a 0 get_a",          1);
    EVAL_INT(i,    "[1 2 3 4 5] 1 2 [] replace_a 1 get_a",          4);

    EVAL_DEPTH(i,  "[1 2 3] 0 1 [9] replace_a",                     1);

    // Doc example: (computer) 1 5 (are) replace -> (career)
    EVAL_INT(i,    "(computer) 1 5 (are) replace_s length_s",       6);
    EVAL_INT(i,    "(computer) 1 5 (are) replace_s 0 get_s",        'c');
    EVAL_INT(i,    "(computer) 1 5 (are) replace_s 1 get_s",        'a');
    EVAL_INT(i,    "(computer) 1 5 (are) replace_s 2 get_s",        'r');
    EVAL_INT(i,    "(computer) 1 5 (are) replace_s 3 get_s",        'e');
    EVAL_INT(i,    "(computer) 1 5 (are) replace_s 5 get_s",        'r');

    // Shorter replacement (hello → hXo).
    EVAL_INT(i,    "(hello) 1 3 (X) replace_s length_s",            3);
    EVAL_INT(i,    "(hello) 1 3 (X) replace_s 1 get_s",             'X');
    EVAL_INT(i,    "(hello) 1 3 (X) replace_s 2 get_s",             'o');

    EVAL_DEPTH(i,  "(abc) 0 1 (X) replace_s",                       1);

    // -------- erase_a / erase_s / erase_p ------------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc:
    //   c1 idx n erase_<a|p|s> -> c1'
    // Same validation as replace; n clamps when idx+n > size.

    // Doc example: [1 2 3 4 5 6] 5 1 erase -> [1 2 3 4 5]
    EVAL_INT(i,    "[1 2 3 4 5 6] 5 1 erase_a length_a",            5);
    EVAL_INT(i,    "[1 2 3 4 5 6] 5 1 erase_a 4 get_a",             5);
    EVAL_INT(i,    "[1 2 3] 0 1 erase_a length_a",                  2);
    EVAL_INT(i,    "[1 2 3] 0 1 erase_a 0 get_a",                   2);
    EVAL_INT(i,    "[1 2 3 4 5] 1 2 erase_a length_a",              3);
    EVAL_INT(i,    "[1 2 3 4 5] 1 2 erase_a 0 get_a",               1);
    EVAL_INT(i,    "[1 2 3 4 5] 1 2 erase_a 1 get_a",               4);
    EVAL_INT(i,    "[1 2 3] 0 0 erase_a length_a",                  3);  // erase 0
    EVAL_INT(i,    "[1 2 3] 0 0 erase_a 0 get_a",                   1);
    EVAL_DEPTH(i,  "[1 2] 0 1 erase_a",                             1);

    // String erase
    EVAL_INT(i,    "(hello) 1 3 erase_s length_s",                  2);
    EVAL_INT(i,    "(hello) 1 3 erase_s 0 get_s",                   'h');
    EVAL_INT(i,    "(hello) 1 3 erase_s 1 get_s",                   'o');
    EVAL_INT(i,    "(hello) 0 1 erase_s 0 get_s",                   'e');
    EVAL_INT(i,    "(abc) 0 0 erase_s length_s",                    3);

    // Procedure erase
    EVAL_INT(i,    "{1 2 3 4} 1 2 erase_p length_p",                2);
    EVAL_INT(i,    "{1 2 3 4 5} 0 1 erase_p length_p",              4);
    EVAL_DEPTH(i,  "{1 2 3} 0 1 erase_p",                           1);

    // -------- insertelement_a / insertelement_s ------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc:
    //   arr idx tok insertelement_a -> arr'   (insert any Token at idx)
    //   str idx ch  insertelement_s -> str'   (insert one char from int)
    //   idx < 0 || idx >= size -> RangeCheckError

    EVAL_INT(i,    "[10 20 30] 1 99 insertelement_a length_a",      4);
    EVAL_INT(i,    "[10 20 30] 1 99 insertelement_a 0 get_a",      10);
    EVAL_INT(i,    "[10 20 30] 1 99 insertelement_a 1 get_a",      99);
    EVAL_INT(i,    "[10 20 30] 1 99 insertelement_a 2 get_a",      20);
    EVAL_INT(i,    "[10 20 30] 1 99 insertelement_a 3 get_a",      30);
    EVAL_INT(i,    "[10 20 30] 0 99 insertelement_a 0 get_a",      99);  // prepend
    EVAL_INT(i,    "[1] 0 42 insertelement_a length_a",             2);
    EVAL_DEPTH(i,  "[1 2] 0 9 insertelement_a",                     1);

    // String element insertion: 88 == 'X'
    EVAL_INT(i,    "(hello) 1 88 insertelement_s length_s",         6);
    EVAL_INT(i,    "(hello) 1 88 insertelement_s 0 get_s",          'h');
    EVAL_INT(i,    "(hello) 1 88 insertelement_s 1 get_s",          'X');
    EVAL_INT(i,    "(hello) 1 88 insertelement_s 2 get_s",          'e');
    EVAL_INT(i,    "(abc) 0 90 insertelement_s 0 get_s",            'Z');  // prepend
    EVAL_DEPTH(i,  "(ab) 0 67 insertelement_s",                     1);

    // -------- prepend_a / prepend_s / prepend_p ------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc: c1 any prepend -> c1'
    // No validation; element goes at index 0.

    EVAL_INT(i,    "[10 20] 99 prepend_a length_a",                 3);
    EVAL_INT(i,    "[10 20] 99 prepend_a 0 get_a",                 99);
    EVAL_INT(i,    "[10 20] 99 prepend_a 1 get_a",                 10);
    EVAL_INT(i,    "[10 20] 99 prepend_a 2 get_a",                 20);
    EVAL_INT(i,    "[] 42 prepend_a length_a",                      1);
    EVAL_INT(i,    "[] 42 prepend_a 0 get_a",                      42);
    EVAL_DEPTH(i,  "[1] 9 prepend_a",                               1);

    EVAL_INT(i,    "{1 2} 99 prepend_p length_p",                   3);
    EVAL_INT(i,    "{} 42 prepend_p length_p",                      1);
    EVAL_DEPTH(i,  "{1} 9 prepend_p",                               1);

    EVAL_INT(i,    "(bc) 65 prepend_s length_s",                    3);  // 'A'+"bc"
    EVAL_INT(i,    "(bc) 65 prepend_s 0 get_s",                     'A');
    EVAL_INT(i,    "(bc) 65 prepend_s 1 get_s",                     'b');
    EVAL_INT(i,    "() 88 prepend_s length_s",                      1);  // 'X'
    EVAL_INT(i,    "() 88 prepend_s 0 get_s",                       'X');
    EVAL_DEPTH(i,  "(a) 66 prepend_s",                              1);

    // -------- size_a / size_s ------------------------------------
    // Different from length: size LEAVES the source on the stack and
    // pushes the size on top. NEST 2.20.2 sli/slidata.cc Size_aFunction.
    //   [1 2 3] size_a -> [1 2 3] 3
    EVAL_DEPTH(i,  "[1 2 3] size_a",                                2);
    EVAL_DEPTH(i,  "(hello) size_s",                                2);
    // Peel off the source via exch pop to verify the size value.
    EVAL_INT(i,    "[1 2 3] size_a exch pop",                       3);
    EVAL_INT(i,    "[] size_a exch pop",                            0);
    EVAL_INT(i,    "[10 20 30 40 50] size_a exch pop",              5);
    EVAL_INT(i,    "(hello) size_s exch pop",                       5);
    EVAL_INT(i,    "() size_s exch pop",                            0);

    // -------- dict literal: << k v ... >> -------------------------
    // `<<` pushes a mark; `>>` collects all key/value pairs above
    // the mark into a fresh Dictionary. NEST 2.20.2 sli/slidict.cc
    // DictconstructFunction.
    EVAL_INT(i,    "<< >> length_d",                                0);
    EVAL_INT(i,    "<< /a 1 >> length_d",                           1);
    EVAL_INT(i,    "<< /a 1 /b 2 /c 3 >> length_d",                 3);
    EVAL_DEPTH(i,  "<< /a 1 /b 2 >>",                               1);
    // Round-trip: build, look up a key.
    EVAL_INT(i,    "<< /x 42 /y 99 >> /x get_d",                    42);
    EVAL_INT(i,    "<< /x 42 /y 99 >> /y get_d",                    99);

    // -------- ostrstream / str -----------------------------------
    // ostrstream creates a std::ostringstream-backed ostream; str
    // extracts its accumulated contents as a string.
    //   ostrstream -> ostream true   (success branch only in modern C++)
    EVAL_DEPTH(i,  "ostrstream",                                    2);
    // Pipeline matches sli-init.sli's `cvs` body: `x ostrstream ; exch <- str`
    // where ; is pop. With value x already on stack:
    //   ostrstream -> stream true; ; (pop true); exch -> [stream, x];
    //   <- pops x (writes it to stream), leaves stream on top; str -> "x".
    EVAL_STRING(i, "42 ostrstream pop exch <- str",                 "42");
    EVAL_STRING(i, "(hello) ostrstream pop exch <- str",            "hello");
    EVAL_STRING(i, "ostrstream pop str",                            "");

    // -------- dictionary helpers: keys / values / cva_d ----------
    EVAL_INT(i,    "<< >> keys length_a",                          0);
    EVAL_INT(i,    "<< /a 1 >> keys length_a",                     1);
    EVAL_INT(i,    "<< /a 1 /b 2 /c 3 >> keys length_a",           3);

    EVAL_INT(i,    "<< /a 1 /b 2 >> values length_a",              2);
    EVAL_INT(i,    "<< /x 42 >> values 0 get_a",                   42);

    // cva_d: dict -> [/k1 v1 /k2 v2 ...]
    EVAL_INT(i,    "<< /a 1 /b 2 >> cva_d length_a",               4);
    EVAL_INT(i,    "<< >> cva_d length_a",                         0);

    // -------- cleardict / clonedict ------------------------------
    EVAL_INT(i,    "<< /a 1 /b 2 >> dup cleardict length_d",       0);
    EVAL_DEPTH(i,  "<< /a 1 >> clonedict",                         2);
    // clonedict leaves [src, copy]; peel to verify the copy size.
    EVAL_INT(i,    "<< /a 1 /b 2 /c 3 >> clonedict exch pop length_d", 3);

    // -------- info_d / topinfo_d / info_ds (stream output) -------
    // Use ostrstream + str to capture into a string. info_d eats
    // both stream and dict; we duplicate the stream first via `2 copy`
    // so it survives for str. Just check the captured string is
    // non-empty (formatting tested by NEST's own test suite).
    EVAL_BOOL(i,
        "<< /x 42 >> ostrstream pop exch 2 copy info_d pop str length_s 0 gt_ii",
        true);

    // -------- call (oosupport) -----------------------------------
    // `dict key call`: look up `key` in `dict`, push `dict` onto the
    // dictstack, execute the resulting name, then pop the dict back
    // off. Non-executable values just land on the operand stack;
    // procedures get executed with `dict` as the current namespace.
    // From NEST 2.20.2 sli/oosupport.cc CallMemberFunction.
    EVAL_INT(i,    "<< /x 42 >> /x call",                           42);
    EVAL_STRING(i, "<< /msg (hi) >> /msg call",                     "hi");
    // Procedure value: dict-resident proc executed in dict's scope.
    EVAL_INT(i,    "<< /two { 1 1 add_ii } >> /two call",            2);

    // -------- search_a -------------------------------------------
    // Semantics from NEST 2.20.2 sli/slidata.cc Search_aFunction:
    //   c1 needle search_a -> post match pre true   (when found)
    //                      -> c1 false              (when not found)

    // Not found: stack ends with [c1, false]; depth 2 disambiguates.
    EVAL_DEPTH(i,  "[1 2 3] [9] search_a",                          2);
    EVAL_BOOL(i,   "[1 2 3] [9] search_a exch pop",                 false);
    EVAL_INT(i,    "[1 2 3] [9] search_a pop length_a",             3);  // c1 intact
    EVAL_INT(i,    "[1 2 3] [9] search_a pop 0 get_a",              1);

    // Found in the middle: stack [post match pre true], depth 4 disambiguates.
    EVAL_DEPTH(i,  "[10 20 30 40 50] [20 30] search_a",             4);
    // Pop bool, pre, match → top is `post` = [40 50]; depth=1 after pops.
    EVAL_INT(i,    "[10 20 30 40 50] [20 30] search_a pop pop pop length_a", 2);
    EVAL_INT(i,    "[10 20 30 40 50] [20 30] search_a pop pop pop 0 get_a", 40);
    // Pop bool, pre → leaves post match; verify match length via exch pop.
    EVAL_INT(i,    "[10 20 30 40 50] [20 30] search_a pop pop length_a exch pop", 2);
    EVAL_INT(i,    "[10 20 30 40 50] [20 30] search_a pop pop 0 get_a exch pop",   20);
    // Pop bool only → post match pre; verify pre length via exch pop exch pop.
    EVAL_INT(i,    "[10 20 30 40 50] [20 30] search_a pop length_a exch pop exch pop", 1);
    EVAL_INT(i,    "[10 20 30 40 50] [20 30] search_a pop 0 get_a exch pop exch pop",  10);

    // Match at start (pre empty).
    EVAL_INT(i,    "[1 2 3] [1] search_a pop length_a exch pop exch pop", 0);
    // Match at end (post empty).
    EVAL_INT(i,    "[1 2 3] [3] search_a pop pop pop length_a",     0);

    // -------- trie round-trip (cva_t / cvt_a) --------------------
    // Build a trie, serialize to array, rebuild, bind, execute.
    // Verifies Cvt_aFunction actually walks the array and rebuilds
    // the trie body (the previous implementation produced an empty
    // root with no entries — calling the rebuilt trie raised
    // StackUnderflow / ArgumentType).
    EVAL_INT(i,
             "/trie_rt_int trie [/integertype] { 1 add_ii } addtotrie "
             "cva_t cvt_a def "
             "5 trie_rt_int",
             6);

    // Two arms: integer maps via add_ii, double via add_dd. Trip
    // through cva_t/cvt_a and check both arms still dispatch.
    EVAL_INT(i,
             "/trie_rt_poly trie "
             "[/integertype] { 1 add_ii } addtotrie "
             "[/doubletype]  { 1.5 add_dd } addtotrie "
             "cva_t cvt_a def "
             "10 trie_rt_poly",
             11);
    EVAL_DOUBLE(i,
                "2.5 trie_rt_poly",
                4.0, 1e-12);

    // Two-parameter trie with alt branching: covers the recursive
    // `[/type [next] [alt]]` layout in both directions.
    EVAL_INT(i,
             "/trie_rt_2p trie "
             "[/integertype /integertype] /add load addtotrie "
             "[/integertype /doubletype]  /mul load addtotrie "
             "cva_t cvt_a def "
             "3 4 trie_rt_2p",
             7);
    EVAL_DOUBLE(i,
                "3 4.0 trie_rt_2p",
                12.0, 1e-12);

    // anytype must remain at the tail of the alt-list after a round
    // trip, otherwise a concrete type behind it becomes unreachable.
    // Insert order here exercises get_alternative's anytype-swap
    // path; after cva_t/cvt_a, the integertype arm must still win
    // for an int operand.
    EVAL_INT(i,
             "/trie_rt_any trie "
             "[/anytype]     { pop 999 } addtotrie "
             "[/integertype] { pop 1 } addtotrie "
             "cva_t cvt_a def "
             "42 trie_rt_any",
             1);
    EVAL_INT(i,
             "(s) trie_rt_any",
             999);

    std::cout << "test_sli_eval: all assertions passed\n";
    return 0;
}
