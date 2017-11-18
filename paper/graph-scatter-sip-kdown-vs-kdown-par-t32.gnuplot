# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.8in,2.0in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-scatter-sip-kdown-vs-kdown-par-t32.tex"

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

load "magma.pal"

set title 'Large'

set xrange [1:2e5]
set yrange [1:2e5]
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
    "<shuf ../experiments/gpgnode-results/sip/runtimes.data" u ($2>=1e5?2e5:$2):($4>=1e5?2e5:$4) w p pt 6 ps 0.2 lc rgb '#812581', \
   x w l ls 0 notitle

