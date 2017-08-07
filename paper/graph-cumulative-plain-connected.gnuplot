# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2.3in font '\footnotesize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-cumulative-plain-connected.tex"

load "magma.pal"

set title 'Unlabelled, connected'

set xrange [1:1e5]
set yrange [0:4110]
set xlabel "Runtime (ms)"
set ylabel "Number solved"
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set key top left
set format x '$10^{%T}$'
set ytics add ('4110' 4110) add ('' 4000)

plot \
    "../experiments/gpgnode-results/mcsplain-connected/runtimes.data" u 2:($2>=1e5?1e-10:1) smooth cumulative w l ti 'Clique' ls 2, \
    "../experiments/gpgnode-results/mcsplain-connected/runtimes.data" u 3:($3>=1e5?1e-10:1) smooth cumulative w l ti 'McSplit' ls 5, \
    "../experiments/gpgnode-results/mcsplain-connected/runtimes.data" u 4:($4>=1e5?1e-10:1) smooth cumulative w l notitle ls 2 dt '.', \
    "../experiments/gpgnode-results/mcsplain-connected/runtimes.data" u 5:($5>=1e5?1e-10:1) smooth cumulative w l notitle ls 5 dt '.'

