(set-logic SLIA)
(set-option :strings-exp true)
(set-option :strings-fmf true)
(set-option :fmf-bound true)
(set-info :status sat)
(declare-fun x () String)
(assert (forall ((n Int)) (=> (and (<= 0 n) (< n (str.len x))) 
(and
(<= 60 (str.code (str.at x n)))
(<= (str.code (str.at x n)) 90)
(< (+ 1 (str.code (str.at x (- n 1)))) (str.code (str.at x n)))
))))
(assert (> (str.len x) 3))
(check-sat)
