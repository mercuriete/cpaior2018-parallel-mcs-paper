# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2in font '\footnotesize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-as-sip.tex"

load "magma.pal"

set title 'Large'

set xrange [1:1e5]
set xlabel "Sequential Runtime (ms)"
set ylabel "Aggregate speedup"
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set key top left

set format x '%.0f'
set format y '%.0f'

set table 'gen-as-sip-kdown-seq.data'
plot "../experiments/gpgnode-results/sip/runtimes.data" u 3:($3>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-sip-mcsplit-seq.data'
plot "../experiments/gpgnode-results/sip/runtimes.data" u 2:($2>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-sip-kdown-par.data'
plot "../experiments/gpgnode-results/sip/runtimes.data" u 5:($5>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-sip-mcsplit-par.data'
plot "../experiments/gpgnode-results/sip/runtimes.data" u 4:($4>=1e5?1e-10:1) smooth cumulative

unset table

set format x '$10^{%T}$'
unset format y
set yrange [0:30]

plot \
    '<./asify.sh gen-as-sip-mcsplit-par.data gen-as-sip-mcsplit-seq.data' u 3:($3/$2) w l title 'McSplit' ls 5, \
    '<./asify.sh gen-as-sip-kdown-par.data gen-as-sip-kdown-seq.data' u 3:($3/$2) w l title 'k${\downarrow}$' ls 8


