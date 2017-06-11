# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2.3in font '\footnotesize'
set output "gen-graph-scatter-33v-mcsplit-vs-mcsplit-par-t32.tex"

load "magma.pal"

set title 'McSplit, Vertex labelled'

set xrange [1:2e5]
set yrange [1:2e5]
set xlabel "Sequential Runtime (ms)"
set ylabel "32 Threads Runtime (ms)"
set logscale xy
set border 3
set grid
set xtics nomirror
set ytics nomirror
set size square
set key off
set format x '$10^{%T}$'
set format y '$10^{%T}$'

set xtics add ('' 1e5) add ('fail' 2e5)
set ytics add ('' 1e5) add ('fail' 2e5)

set palette negative
set cbrange [0:5]
unset colorbox

plot \
    "<shuf ../experiments/gpgnode-results/mcs33v/runtimes.data" u ($3>=1e5?2e5:$3):($11>=1e5?2e5:$11):((stringcolumn(1))[4:5]eq"10"?1:(stringcolumn(1))[4:5]eq"30"?2:(stringcolumn(1))[4:5]eq"50"?3:(stringcolumn(1))[4:5]eq"70"?4:(stringcolumn(1))[4:5]eq"90"?5:(stringcolumn(1))eq"instance"?1:100) w p pt 6 ps 0.2 lc pal, \
   x w l ls 0 notitle

