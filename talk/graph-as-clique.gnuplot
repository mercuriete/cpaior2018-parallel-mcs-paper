# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 3.5in,2.2in font '\scriptsize' preamble '\input{gnuplot-preamble}'

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

set output "gen-graph-as-clique.tex"

load "paired.pal"

set title 'Clique'

set xrange [1:1e6]
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

set table 'gen-as-alt-33v-clique-seq.data'
plot "../experiments/fatanode-results/mcs33v/runtimes.data" u 2:($2>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33v-clique-par.data'
plot "../experiments/fatanode-results/mcs33v/runtimes.data" u 4:($4>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ve-connected-clique-seq.data'
plot "../experiments/fatanode-results/mcs33ve-connected/runtimes.data" u 2:($2>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ve-connected-clique-par.data'
plot "../experiments/fatanode-results/mcs33ve-connected/runtimes.data" u 4:($4>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ved-clique-seq.data'
plot "../experiments/fatanode-results/mcs33ved/runtimes.data" u 2:($2>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ved-clique-par.data'
plot "../experiments/fatanode-results/mcs33ved/runtimes.data" u 4:($4>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-connected-clique-seq.data'
plot "../experiments/fatanode-results/mcsplain-connected/runtimes.data" u 2:($2>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-connected-clique-par.data'
plot "../experiments/fatanode-results/mcsplain-connected/runtimes.data" u 4:($4>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-clique-seq.data'
plot "../experiments/fatanode-results/mcsplain/runtimes.data" u 2:($2>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-clique-par.data'
plot "../experiments/fatanode-results/mcsplain/runtimes.data" u 5:($5>=1e6?1e-10:1) smooth cumulative

unset table

set format x '$10^{%T}$'
unset format y
set yrange [0:50]
set ytics add ('$1$' 1) ('' 0) ('$32$' 32)

plot \
    '<../paper/asify.sh gen-as-alt-plain-clique-par.data gen-as-alt-plain-clique-seq.data' u 3:($3/$2) w l title 'Unlabelled' at end ls 1, \
    '<../paper/asify.sh gen-as-alt-33v-clique-par.data gen-as-alt-33v-clique-seq.data' u 3:($3/$2) w l title 'Vertex' at end ls 2, \
    '<../paper/asify.sh gen-as-alt-33ved-clique-par.data gen-as-alt-33ved-clique-seq.data' u 3:($3/$2) w l title 'Both, directed' at end ls 4, \
    '<../paper/asify.sh gen-as-alt-plain-connected-clique-par.data gen-as-alt-plain-connected-clique-seq.data' u 3:($3/$2) w l title 'Unlabelled, connected' at end ls 6, \
    '<../paper/asify.sh gen-as-alt-33ve-connected-clique-par.data gen-as-alt-33ve-connected-clique-seq.data' u 3:($3/$2) w l title 'Both, connected' at end ls 7

