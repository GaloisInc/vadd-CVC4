; COMMAND-LINE: --decision=justification
; EXPECT: unsat

(benchmark pp_regfile.smt
:logic QF_AUFLIA
:extrafuns ((REGFILE_INIT Array))
:extrafuns ((BDEST_S2E_INIT Int))
:extrafuns ((IMEM_INIT Array))
:extrafuns ((OPCODE_OF Int Int))
:status unknown
:formula
(let (?n1 0)
(let (?n2 (select REGFILE_INIT ?n1))
(let (?n3 (select IMEM_INIT ?n1))
(let (?n4 (OPCODE_OF ?n3))
(let (?n5 1)
(flet ($n6 (= ?n4 ?n5))
(flet ($n7 (= ?n1 ?n3))
(flet ($n8 false)
(flet ($n9 true)
(flet ($n10 (if_then_else $n7 $n8 $n9))
(flet ($n11 (= BDEST_S2E_INIT ?n1))
(flet ($n12 (if_then_else $n11 $n8 $n9))
(let (?n13 (store REGFILE_INIT BDEST_S2E_INIT ?n1))
(let (?n14 (ite $n12 ?n13 REGFILE_INIT))
(let (?n15 (store ?n14 ?n3 ?n1))
(let (?n16 (ite $n10 ?n15 ?n14))
(flet ($n17 (= ?n1 ?n4))
(let (?n18 16)
(flet ($n19 (= ?n18 ?n4))
(let (?n20 (ite $n19 ?n16 ?n14))
(let (?n21 (ite $n17 ?n14 ?n20))
(let (?n22 (ite $n6 ?n16 ?n21))
(let (?n23 (select ?n22 ?n1))
(flet ($n24 (= ?n2 ?n23))
(flet ($n25 (not $n24))
$n25
))))))))))))))))))))))))))
