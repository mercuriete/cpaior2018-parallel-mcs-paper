# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2in font '\footnotesize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-as-33ved.tex"

load "magma.pal"

set title 'Fully labelled, directed'

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

set table 'gen-as-33ved-clique-seq.data'
plot "../experiments/gpgnode-results/mcs33ved/runtimes.data" u 2:($2>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-33ved-mcsplit-seq.data'
plot "../experiments/gpgnode-results/mcs33ved/runtimes.data" u 3:($3>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-33ved-clique-par.data'
plot "../experiments/gpgnode-results/mcs33ved/runtimes.data" u 6:($6>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-33ved-mcsplit-par.data'
plot "../experiments/gpgnode-results/mcs33ved/runtimes.data" u 11:($11>=1e5?1e-10:1) smooth cumulative

unset table

set format x '$10^{%T}$'
unset format y
set yrange [0:30]

plot \
    '<./asify.sh gen-as-33ved-clique-par.data gen-as-33ved-clique-seq.data' u 3:($3/$2) w l title 'Clique' ls 2, \
    '<./asify.sh gen-as-33ved-mcsplit-par.data gen-as-33ved-mcsplit-seq.data' u 3:($3/$2) w l title 'McSplit' ls 5


