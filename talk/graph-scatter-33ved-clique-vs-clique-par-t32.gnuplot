# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.9in,1.6in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-scatter-33ved-clique-vs-clique-par-t32.tex"

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

load "magma.pal"

set title 'Clique, Labelled'

set xrange [1:2e6]
set yrange [1:2e6]
set logscale xy
set border 3
set grid
set xtics nomirror
set ytics nomirror
set size square
set key off
set format x '$10^{%T}$'
set format y '$10^{%T}$'
set xtics add ('' 1e6) add ('fail' 2e6)
set ytics add ('' 1e6) add ('fail' 2e6)

set palette negative
set cbrange [0:5]
unset colorbox

plot \
    "<shuf ../experiments/fatanode-results/mcs33ved/runtimes.data" u ($2>=1e6?2e6:$2):($4>=1e6?2e6:$4):((stringcolumn(1))[4:5]eq"10"?1:(stringcolumn(1))[4:5]eq"30"?2:(stringcolumn(1))[4:5]eq"50"?3:(stringcolumn(1))[4:5]eq"70"?4:(stringcolumn(1))[4:5]eq"90"?5:(stringcolumn(1))eq"instance"?1:100) w p pt 6 ps 0.2 lc pal, \
   x w l ls 0 notitle

