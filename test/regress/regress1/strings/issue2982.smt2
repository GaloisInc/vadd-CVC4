; COMMAND-LINE: --strings-exp --re-elim --re-elim-agg
; EXPECT: unsat
(set-logic QF_SLIA)

(declare-fun var_0 () String)
(declare-fun var_1 () String)
(declare-fun var_2 () String)
(declare-fun var_3 () String)
(declare-fun var_4 () String)
(declare-fun var_5 () String)
(declare-fun var_6 () String)
(declare-fun var_7 () String)
(declare-fun var_8 () String)
(declare-fun var_9 () String)
(declare-fun var_10 () String)
(declare-fun var_11 () String)
(declare-fun var_12 () String)

(assert (str.in.re (str.++ var_7 "z" var_7 ) (re.* (str.to.re "z"))))
(assert (str.in.re var_7 (re.* (re.range "a" "u"))))
(assert (not (str.in.re (str.++ "a" var_7 "z" "a" var_7 ) (re.++ (re.* (re.union (str.to.re "z") (re.++ (str.to.re "a") (re.++ (re.* (str.to.re "a")) (str.to.re "z"))))) (re.++ (str.to.re "a") (re.* (str.to.re "a")))))))
(assert (and (<= (str.len var_7) 0 ) (<= 0  (str.len var_7))))
(check-sat)
