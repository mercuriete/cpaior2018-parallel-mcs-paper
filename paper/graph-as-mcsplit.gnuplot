# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,2.5in font '\scriptsize' preamble '\input{gnuplot-preamble}'

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

set output "gen-graph-as-mcsplit.tex"

load "magma.pal"

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

set table 'gen-as-alt-33v-mcsplit-seq.data'
plot "../experiments/fatanode-results/mcs33v/runtimes.data" u 3:($3>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33v-mcsplit-par.data'
plot "../experiments/fatanode-results/mcs33v/runtimes.data" u 5:($5>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ve-connected-mcsplit-seq.data'
plot "../experiments/fatanode-results/mcs33ve-connected/runtimes.data" u 3:($3>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ve-connected-mcsplit-par.data'
plot "../experiments/fatanode-results/mcs33ve-connected/runtimes.data" u 5:($5>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ved-mcsplit-seq.data'
plot "../experiments/fatanode-results/mcs33ved/runtimes.data" u 3:($3>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-33ved-mcsplit-par.data'
plot "../experiments/fatanode-results/mcs33ved/runtimes.data" u 5:($5>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-connected-mcsplit-seq.data'
plot "../experiments/fatanode-results/mcsplain-connected/runtimes.data" u 3:($3>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-connected-mcsplit-par.data'
plot "../experiments/fatanode-results/mcsplain-connected/runtimes.data" u 5:($5>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-mcsplit-seq.data'
plot "../experiments/fatanode-results/mcsplain/runtimes.data" u 3:($3>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-mcsplit-par.data'
plot "../experiments/fatanode-results/mcsplain/runtimes.data" u 6:($6>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-sip-mcsplit-seq.data'
plot "../experiments/fatanode-results/sip/runtimes.data" u 3:($3>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-sip-mcsplit-par.data'
plot "../experiments/fatanode-results/sip/runtimes.data" u 5:($5>=1e6?1e-10:1) smooth cumulative

unset table

set format x '$10^{%T}$'
unset format y
set yrange [0:20]
set ytics add ('$1$' 1) ('' 0)

plot \
    '<./asify.sh gen-as-alt-plain-mcsplit-par.data gen-as-alt-plain-mcsplit-seq.data' u 3:($3/$2) w l title 'Unlabelled' at end ls 1, \
    '<./asify.sh gen-as-alt-33v-mcsplit-par.data gen-as-alt-33v-mcsplit-seq.data' u 3:($3/$2) w l title '\raisebox{0mm}{Vertex}' at end ls 2, \
    '<./asify.sh gen-as-alt-33ved-mcsplit-par.data gen-as-alt-33ved-mcsplit-seq.data' u 3:($3/$2) w l title '\raisebox{0mm}{Both, dir.}' at end ls 4, \
    '<./asify.sh gen-as-alt-plain-connected-mcsplit-par.data gen-as-alt-plain-connected-mcsplit-seq.data' u 3:($3/$2) w l title '\raisebox{2mm}{Unlabelled, conn.}' at end ls 6, \
    '<./asify.sh gen-as-alt-33ve-connected-mcsplit-par.data gen-as-alt-33ve-connected-mcsplit-seq.data' u 3:($3/$2) w l title 'Both, conn.' at end ls 7, \
    '<./asify.sh gen-as-alt-sip-mcsplit-par.data gen-as-alt-sip-mcsplit-seq.data' u 3:($3/$2) w l title 'Large' at end ls 8

