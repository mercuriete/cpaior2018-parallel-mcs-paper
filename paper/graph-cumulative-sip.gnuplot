# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2.3in font '\footnotesize'
set output "gen-graph-cumulative-sip.tex"

load "magma.pal"

set title 'Large'

set xrange [1:1e5]
set yrange [0:5725]
set xlabel "Runtime (ms)"
set ylabel "Number solved"
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set key top left
set format x '$10^{%T}$'
set ytics add ('5725' 5725)

plot \
    "../experiments/gpgnode-results/sip/runtimes.data" u 2:($2>=1e5?1e-10:1) smooth cumulative w l ti 'McSplit${\downarrow}$' ls 5, \
    "../experiments/gpgnode-results/sip/runtimes.data" u 3:($3>=1e5?1e-10:1) smooth cumulative w l ti 'k${\downarrow}$' ls 8

