
cd out

# 生成perf.data
perf record -F 200 -a -g -- sleep 120

# dump
perf script > out.perf

# 折叠调用栈
../stackcollapse-perf.pl out.perf > out.folded

# 生成火焰图
../flamegraph.pl out.folded > out.svg