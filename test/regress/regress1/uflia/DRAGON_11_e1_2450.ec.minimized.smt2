; COMMAND-LINE: --incremental
; EXPECT: sat
; EXPECT: sat

(set-logic QF_UFLIA)
(declare-fun _base () Int)
(declare-fun _n () Int)
(assert (let ((.def_5 (<= 0 _n)))
.def_5
))
(declare-fun ___z2z___ (Int) Bool)
(declare-fun ___z3z___ (Int) Bool)
(declare-fun ___z4z___ (Int) Bool)
(declare-fun ___z5z___ (Int) Bool)
(declare-fun ___z6z___ (Int) Bool)
(declare-fun ___z7z___ (Int) Bool)
(declare-fun ___z8z___ (Int) Bool)
(declare-fun ___z9z___ (Int) Bool)
(declare-fun ___z10z___ (Int) Bool)
(declare-fun ___z11z___ (Int) Bool)
(declare-fun ___z12z___ (Int) Bool)
(declare-fun ___z13z___ (Int) Bool)
(declare-fun ___z14z___ (Int) Int)
(declare-fun ___z15z___ (Int) Bool)
(declare-fun ___z16z___ (Int) Int)
(declare-fun ___z17z___ (Int) Int)
(declare-fun ___z18z___ (Int) Int)
(declare-fun ___z19z___ (Int) Int)
(declare-fun ___z20z___ (Int) Int)
(declare-fun ___z22z___ (Int) Bool)
(declare-fun ___z23z___ (Int) Bool)
(declare-fun ___z24z___ (Int) Bool)
(declare-fun ___z25z___ (Int) Bool)
(declare-fun ___z26z___ (Int) Bool)
(declare-fun ___z27z___ (Int) Bool)
(declare-fun ___z28z___ (Int) Bool)
(declare-fun ___z29z___ (Int) Bool)
(declare-fun ___z30z___ (Int) Bool)
(declare-fun ___z31z___ (Int) Bool)
(declare-fun ___z32z___ (Int) Bool)
(declare-fun ___z33z___ (Int) Bool)
(declare-fun ___z34z___ (Int) Bool)
(declare-fun ___z35z___ (Int) Int)
(declare-fun ___z36z___ (Int) Bool)
(declare-fun ___z37z___ (Int) Int)


(assert (let ((.def_1132 (___z29z___ (- 1))))
(let ((.def_1126 (___z20z___ (- 2))))
(let ((.def_1127 (<= 1 .def_1126)))
(let ((.def_1124 (___z19z___ (- 2))))
(let ((.def_1125 (= .def_1124 0)))
(let ((.def_1128 (and .def_1125 .def_1127)))
(let ((.def_1106 (___z17z___ (- 2))))
(let ((.def_1107 (= .def_1106 0)))
(let ((.def_1129 (and .def_1107 .def_1128)))
(let ((.def_1122 (___z16z___ (- 2))))
(let ((.def_1123 (= .def_1122 0)))
(let ((.def_1130 (and .def_1123 .def_1129)))
(let ((.def_1108 (___z18z___ (- 2))))
(let ((.def_1114 (= .def_1108 0)))
(let ((.def_1131 (and .def_1114 .def_1130)))
(let ((.def_1133 (= .def_1131 .def_1132)))
.def_1133
)))))))))))))))))
(push 1)
(check-sat)
(pop 1)
(assert (let ((.def_2016 (___z16z___ (- 3))))
(let ((.def_2188 (+ (- 1) .def_2016)))
(let ((.def_2048 (___z34z___ (- 2))))
(let ((.def_2189 (ite .def_2048 .def_2188 .def_2016)))
(let ((.def_2057 (___z13z___ (- 2))))
(let ((.def_2190 (ite .def_2057 .def_2189 .def_2016)))
(let ((.def_2036 (___z30z___ (- 2))))
(let ((.def_2191 (ite .def_2036 0 .def_2016)))
(let ((.def_2064 (___z9z___ (- 2))))
(let ((.def_2192 (ite .def_2064 .def_2191 .def_2190)))
(let ((.def_2193 (___z25z___ (- 2))))
(let ((.def_2194 (ite .def_2193 .def_2188 .def_2016)))
(let ((.def_2074 (___z4z___ (- 2))))
(let ((.def_2195 (ite .def_2074 .def_2194 .def_2192)))
(let ((.def_2196 (___z24z___ (- 2))))
(let ((.def_2197 (ite .def_2196 0 .def_2016)))
(let ((.def_2076 (___z3z___ (- 2))))
(let ((.def_2198 (ite .def_2076 .def_2197 .def_2195)))
(let ((.def_2199 (+ 1 .def_2016)))
(let ((.def_2200 (___z23z___ (- 2))))
(let ((.def_2201 (ite .def_2200 .def_2199 .def_2016)))
(let ((.def_2078 (___z2z___ (- 2))))
(let ((.def_2202 (ite .def_2078 .def_2201 .def_2198)))
(let ((.def_2052 (= _base (- 2))))
(let ((.def_2203 (ite .def_2052 0 .def_2202)))
(let ((.def_1122 (___z16z___ (- 2))))
(let ((.def_2204 (= .def_1122 .def_2203)))
.def_2204
))))))))))))))))))))))))))))

(push 1)
(assert (let ((.def_2052 (= _base (- 2))))
(let ((.def_2278 (not .def_2052)))
(let ((.def_997 (___z15z___ 0)))
(let ((.def_2290 (or .def_997 .def_2278)))
(let ((.def_2291 (not .def_2290)))
.def_2291
))))))
(check-sat)
(pop 1)
