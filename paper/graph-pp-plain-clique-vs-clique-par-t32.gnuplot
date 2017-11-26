# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.6in,1.4in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-pp-plain-clique-vs-clique-par-t32.tex"

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
set xtics add ('1' 1) add ('10' 10)
set arrow 1 from 1, 0 to 1, 3000 lc rgb '#a0a0a0' back nohead

seqcol=2
parcol=5
starttime=5e2
runtimes="../experiments/fatanode-results/mcsplain/runtimes.data"

set label 2 at 0.3, 2600 right 'All' front
set arrow 2 from 0.3, 2600 to 3, 2600 ls 1 front

set label 3 at 3e0, 800 center 'Hard' front
set arrow 3 from 3e0, 1000 to 8e-1, 1600 ls 6 front

set label 4 at 2.7e-2, 1880 right 'PAR10' front
set arrow 4 from 2e-2, 1800 to 1e-1, 1600 ls 8 front

plot \
    runtimes u ((column(seqcol)>=1e6&&column(parcol)>=1e6)?1:(column(parcol)==0?1:column(parcol)>=1e6?1e6:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e6?1e6:column(seqcol))):((column(seqcol)>=1e6&&column(parcol)>=1e6)?0:1) smooth cum w l ls 1, \
    runtimes u (column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?1:(column(parcol)==0?1:column(parcol)>=1e6?1e6:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e6?1e6:column(seqcol))):(column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?0:1) smooth cum w l ls 6, \
    runtimes u (column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?1:(column(parcol)==0?1:column(parcol)>=1e6?1e7:column(parcol))/(column(seqcol)==0?1:column(seqcol)>=1e6?1e7:column(seqcol))):(column(seqcol)<starttime||(column(seqcol)>=1e6&&column(parcol)>=1e6)?0:1) smooth cum w l ls 8
