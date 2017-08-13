# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2.2in font '\footnotesize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-cumulative-33v.tex"

load "magma.pal"

set title 'Vertex labelled'

set xrange [1:1e5]
set yrange [0:8140]
set xlabel "Runtime (ms)"
set ylabel "Number solved"
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set key bottom right
set format x '$10^{%T}$'
set ytics add ('8140' 8140) add ('' 8000)

plot \
    "../experiments/gpgnode-results/mcs33v/runtimes.data" u 2:($2>=1e5?1e-10:1) smooth cumulative w l ti 'Clique' ls 2, \
    "../experiments/gpgnode-results/mcs33v/runtimes.data" u 3:($3>=1e5?1e-10:1) smooth cumulative w l ti 'McSplit' ls 5, \
    "../experiments/gpgnode-results/mcs33v/runtimes.data" u 6:($6>=1e5?1e-10:1) smooth cumulative w l notitle ls 2 dt '.', \
    "../experiments/gpgnode-results/mcs33v/runtimes.data" u 11:($11>=1e5?1e-10:1) smooth cumulative w l notitle ls 5 dt '.'

