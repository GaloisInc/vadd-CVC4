(set-logic QF_UFLRA)
(set-info :smt-lib-version 2.0)
(set-info :category "crafted")
(set-info :status unsat)
(declare-sort U 0)
(declare-fun a () U)
(declare-fun b () U)
(declare-fun f (U) U)
(declare-fun x () Real)
(declare-fun y () Real)

(assert (= a b))
(assert (or (= x y) (not (= (f a) (f b)))))
(assert (> x (+ y 1)))
(check-sat)
(exit)