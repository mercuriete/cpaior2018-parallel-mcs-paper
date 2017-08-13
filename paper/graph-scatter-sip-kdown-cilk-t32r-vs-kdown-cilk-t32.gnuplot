# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.8in,2.0in font '\footnotesize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-scatter-sip-kdown-cilk-t32r-vs-kdown-cilk-t32.tex"

load "magma.pal"

set title 'Large, 32 vs 32 (Cilk)'

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
    "<shuf ../experiments/gpgnode-results/sip/runtimes.data" u ($8>=1e5?2e5:$8):($7>=1e5?2e5:$7) w p pt 6 ps 0.2 lc rgb '#812581', \
   x w l ls 0 notitle

