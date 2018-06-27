# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.1in,1.8in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-cumulative-sip.tex"

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

load "paired.pal"

set title 'Large'

set xrange [1:1e6]
set yrange [0:5725]
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set key top left
set format x '$10^{%T}$'
set ytics add ('5725' 5725)

plot \
    "../experiments/fatanode-results/sip/runtimes.data" u 2:($2>=1e6?1e-10:1) smooth cumulative w l ti 'k${\downarrow}$' ls 4, \
    "../experiments/fatanode-results/sip/runtimes.data" u 3:($3>=1e6?1e-10:1) smooth cumulative w l ti 'McSplit${\downarrow}$' ls 5, \
    "../experiments/fatanode-results/sip/runtimes.data" u 4:($4>=1e6?1e-10:1) smooth cumulative w l notitle ls 4 dt '.', \
    "../experiments/fatanode-results/sip/runtimes.data" u 5:($5>=1e6?1e-10:1) smooth cumulative w l notitle ls 5 dt '.'

