(set-option :incremental false)
(set-logic ALL)
(declare-sort $$unsorted 0)
(declare-sort a 0)
(declare-fun p (a) Bool)
(assert (not (= (exists ((Xy a)) (= p ((lambda ((Xx a) (Xy a)) (= Xx Xy)) Xy)) ) (exists ((Xy a)) (and (p Xy) (forall ((Xz a)) (=> (p Xz) (= Xy Xz)) )) ))))
(meta-info :filename "SEV386^5")
(check-sat-assuming ( (not false) ))