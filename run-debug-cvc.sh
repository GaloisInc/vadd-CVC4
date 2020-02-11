./debug/bin/cvc4 --proof --dump-proof --no-expand-definitions-pp --no-unconstrained-simp --no-apply-subst --no-static-learning --simplification=none --no-repeat-simp --no-global-negate --no-symmetry-breaker --no-pb-rewrites --no-rewrite-apply-to-const --no-theory-preprocess --no-sort-inference --no-pre-skolem-quant --no-bv-eq --no-bv-ineq --no-bv-algebraic --no-bv-to-bool --bool-to-bv=off --no-bv-intro-pow2 --no-bitblast-aig  "$@"

#### add after ./run-debug-cvc.sh

# solved before theories kick in (only resolution at level 0)
# qf-unsat-01-nary.smt2 --new-proof


# qf-unsat-01-nary.smt2 --new-proof -d theory::explain