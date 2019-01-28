solver=~/cvc/wt-thf/debug/bin/cvc4
options="--uf-ho --full-saturate-quant"
traces=""
time=""
ulimit="ulimit -S -t 2"
file="flat.txt"
path="/home/hbarbosa/benchmarks/sygus/lustre/"

echo "Options: $options"
echo "Traces: $traces"
echo

# while read name; do
#     echo "$solver on $path$name";
#     $ulimit; $time $solver $options $traces  "$path$name" > "flat/$name";
#     echo "=====================================";
# done < $file

for name in ~/benchmarks/tptp/Problems/SEU/*.p; do
    # Ignoring lambdas for now
    if grep -q "\^.\[" "$name"; then
        continue
    fi
    # Only run on problems with thf constructs
    if grep -q thf "$name"; then
        echo "$solver on $name";
        $ulimit; $time $solver $options $traces "$name"
        echo "=====================================";
    fi
done
