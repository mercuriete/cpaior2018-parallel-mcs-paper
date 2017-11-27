# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.4in,1.5in font '\scriptsize' preamble '\input{gnuplot-preamble}'

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

set output "gen-graph-as-kdown.tex"

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

set table 'gen-as-alt-plain-kdown-seq.data'
plot "../experiments/fatanode-results/mcsplain/runtimes.data" u 4:($4>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-plain-kdown-par.data'
plot "../experiments/fatanode-results/mcsplain/runtimes.data" u 7:($7>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-sip-kdown-seq.data'
plot "../experiments/fatanode-results/sip/runtimes.data" u 2:($2>=1e6?1e-10:1) smooth cumulative

set table 'gen-as-alt-sip-kdown-par.data'
plot "../experiments/fatanode-results/sip/runtimes.data" u 4:($4>=1e6?1e-10:1) smooth cumulative

unset table

set format x '$10^{%T}$'
unset format y
set yrange [0:32]
set ytics add ('$1$' 1) ('' 0) ('$32$' 32)

plot \
    '<./asify.sh gen-as-alt-plain-kdown-par.data gen-as-alt-plain-kdown-seq.data' u 3:($3/$2) w l title 'Unlabelled' at end ls 1, \
    '<./asify.sh gen-as-alt-sip-kdown-par.data gen-as-alt-sip-kdown-seq.data' u 3:($3/$2) w l title 'Large' at end ls 8
