# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.6in,1.4in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-pp-plain-connected-clique-vs-clique-par-t32.tex"

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

load "magma.pal"

set xrange [1e-3:1e1]
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set size square
set key off
set format x '$10^{%T}$'

seqcol=2
parcol=4
starttime=5e2
runtimes="../experiments/gpgnode-results/mcsplain-connected/runtimes.data"

plot \
    runtimes u ((column(seqcol)>=1e5&&column(parcol)>=1e5)?1:(column(parcol)==0?1:column(parcol)>=1e5?1e5:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e5?1e5:column(seqcol))):((column(seqcol)>=1e5&&column(parcol)>=1e5)?0:1) smooth cum w l ls 1, \
    runtimes u (column(seqcol)<starttime||(column(seqcol)>=1e5&&column(parcol)>=1e5)?1:(column(parcol)==0?1:column(parcol)>=1e5?1e5:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e5?1e5:column(seqcol))):(column(seqcol)<starttime||(column(seqcol)>=1e5&&column(parcol)>=1e5)?0:1) smooth cum w l ls 6, \
    runtimes u (column(seqcol)<starttime||(column(seqcol)>=1e5&&column(parcol)>=1e5)?1:(column(parcol)==0?1:column(parcol)>=1e5?1e6:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e5?1e6:column(seqcol))):(column(seqcol)<starttime||(column(seqcol)>=1e5&&column(parcol)>=1e5)?0:1) smooth cum w l ls 8

