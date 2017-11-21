# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.6in,1.4in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-pp-33ved-clique-vs-clique-par-t32.tex"

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
set nomytics
set size square
set key off
set format x '$10^{%T}$'
set xtics add ('1' 1) add ('10' 10)
set arrow 1 from 1, 0 to 1, 8000 lc rgb '#a0a0a0' back nohead

seqcol=2
parcol=4
starttime=5e2
runtimes="../experiments/fatanode-results/mcs33ved/runtimes.data"

plot \
    runtimes u ((column(seqcol)>=1e6&&column(parcol)>=1e6)?1:(column(parcol)==0?1:column(parcol)>=1e6?1e6:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e6?1e6:column(seqcol))):((column(seqcol)>=1e6&&column(parcol)>=1e6)?0:1) smooth cum w l ls 1, \
    runtimes u (column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?1:(column(parcol)==0?1:column(parcol)>=1e6?1e6:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e6?1e6:column(seqcol))):(column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?0:1) smooth cum w l ls 6, \
    runtimes u (column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?1:(column(parcol)==0?1:column(parcol)>=1e6?1e7:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e6?1e7:column(seqcol))):(column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?0:1) smooth cum w l ls 8
