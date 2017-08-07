# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2in font '\footnotesize'
set output "gen-graph-as-plain.tex"

load "magma.pal"

set title 'Unlabelled'

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

set table 'gen-as-plain-clique-seq.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 2:($2>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-plain-mcsplit-seq.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 3:($3>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-plain-kdown-seq.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 4:($4>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-plain-clique-par.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 7:($7>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-plain-mcsplit-par.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 12:($12>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-plain-kdown-par.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 15:($15>=1e5?1e-10:1) smooth cumulative

unset table

set format x '$10^{%T}$'
unset format y
set yrange [0:30]

plot \
    '<./asify.sh gen-as-plain-clique-par.data gen-as-plain-clique-seq.data' u 3:($3/$2) w l title 'Clique' ls 2, \
    '<./asify.sh gen-as-plain-mcsplit-par.data gen-as-plain-mcsplit-seq.data' u 3:($3/$2) w l title 'McSplit' ls 5, \
    '<./asify.sh gen-as-plain-kdown-par.data gen-as-plain-kdown-seq.data' u 3:($3/$2) w l title 'k${\downarrow}$' ls 8


