# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,1.5in font '\footnotesize' preamble '\input{gnuplot-preamble}'

set output "gen-graph-as-kdown.tex"

load "magma.pal"

set xrange [1:1e5]
set xlabel "Sequential runtime (ms)"
set ylabel "Aggregate speedup"
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set key off

set format x '%.0f'
set format y '%.0f'

set table 'gen-as-alt-plain-kdown-seq.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 4:($4>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-kdown-par.data'
plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u 16:($16>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-alt-sip-kdown-seq.data'
plot "../experiments/gpgnode-results/sip/runtimes.data" u 2:($2>=1e5?1e-10:1) smooth cumulative

set table 'gen-as-alt-sip-kdown-par.data'
plot "../experiments/gpgnode-results/sip/runtimes.data" u 4:($4>=1e5?1e-10:1) smooth cumulative

unset table

set format x '$10^{%T}$'
unset format y
set yrange [0:20]
set ytics add ('$1$' 1) ('' 0)

plot \
    '<./asify.sh gen-as-alt-plain-kdown-par.data gen-as-alt-plain-kdown-seq.data' u 3:($3/$2) w l title 'Unlabelled' at end ls 1, \
    '<./asify.sh gen-as-alt-sip-kdown-par.data gen-as-alt-sip-kdown-seq.data' u 3:($3/$2) w l title 'Large' at end ls 8

