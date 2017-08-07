# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.8in,2.0in font '\footnotesize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-scatter-33ved-clique-cilk-t8-vs-clique-cilk-t16.tex"

load "magma.pal"

set title 'Clique, Fully labelled'

set xrange [1:2e5]
set yrange [1:2e5]
set xlabel "8 Workers Runtime (ms)"
set ylabel "16 Workers Runtime (ms)"
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
    "<shuf ../experiments/gpgnode-results/mcs33ved/runtimes.data" u ($23>=1e5?2e5:$23):($24>=1e5?2e5:$24):((stringcolumn(1))[4:5]eq"10"?1:(stringcolumn(1))[4:5]eq"30"?2:(stringcolumn(1))[4:5]eq"50"?3:(stringcolumn(1))[4:5]eq"70"?4:(stringcolumn(1))[4:5]eq"90"?5:(stringcolumn(1))eq"instance"?1:100) w p pt 6 ps 0.2 lc pal, \
   x w l ls 0 notitle, \
   x / 2 w l ls 0 title '$2{\times}$' at end

