# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.5in,1.3in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-pp2-33ved-clique-vs-clique-par-t32.tex"

seqcol=2
parcol=4
starttime=5e2
runtimes="../experiments/fatanode-results/mcs33ved/runtimes.data"

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

load "paired.pal"
set title 'Clique, Lab'

set xrange [1:1e4]
set logscale x
set border 3
set grid
set xtics nomirror
set ytics nomirror
set key off
set format x '$10^{%T}$'
set xtics add ('$1$' 1) add ('$10$' 10) add ('$32$' 32) add ('$100$' 100)
set yrange [0:1]

set table "/dev/null"

hardsum=0
allsum=0
plot \
    runtimes u 0:((column(seqcol)<1e6||column(parcol)<1e6)?(allsum=allsum+1,1):(0)), \
    runtimes u 0:(column(seqcol)>=starttime&&(column(seqcol)<1e6||column(parcol)<1e6)?(hardsum=hardsum+1,1):(0)), \

unset table

nc(c)=(column(c)>=1e6?1e6:column(c)<1?1:column(c))
ncpar10(c)=(column(c)>=1e6?1e7:column(c)<1?1:column(c))
hardnc(a,c)=(column(a)<starttime?0:column(c)>=1e6?1e6:column(c)<1?1:column(c))
hardpar10nc(a,c)=(column(a)<starttime?0:column(c)>=1e6?1e7:column(c)<1?1:column(c))
vbs(a,b)=(nc(a)<nc(b)?nc(a):nc(b))
vbspar10(a,b)=(ncpar10(a)<ncpar10(b)?ncpar10(a):ncpar10(b))
score(a,b)=(nc(a)==1e6&&nc(b)==1e6?0:1.0/allsum)
hardscore(a,b)=(nc(a)==1e6&&nc(b)==1e6?0:nc(a)>=starttime?1.0/hardsum:0)

plot \
    runtimes u (nc(seqcol)/vbs(seqcol,parcol)):(score(seqcol,parcol)) smooth cum w l ls 2 ti 'All', \
    runtimes u (nc(parcol)/vbs(seqcol,parcol)):(score(seqcol,parcol)) smooth cum w l ls 2 dt '.' notitle, \
    runtimes u (hardnc(seqcol,seqcol)/vbs(seqcol,parcol)):(hardscore(seqcol,parcol)) smooth cum w l ls 6 ti 'Hard', \
    runtimes u (hardnc(seqcol,parcol)/vbs(seqcol,parcol)):(hardscore(seqcol,parcol)) smooth cum w l ls 6 dt '.' notitle, \
    runtimes u (hardpar10nc(seqcol,seqcol)/vbspar10(seqcol,parcol)):(hardscore(seqcol,parcol)) smooth cum w l ls 8 ti 'PAR10', \
    runtimes u (hardpar10nc(seqcol,parcol)/vbspar10(seqcol,parcol)):(hardscore(seqcol,parcol)) smooth cum w l ls 8 dt '.' notitle, \
    runtimes u (NaN):(NaN) w l lc rgb '#ffffff' ti '~', \
    runtimes u (NaN):(NaN) w l ls 1 ti 'Seq', \
    runtimes u (NaN):(NaN) w l ls 1 dt '.' ti 'Par'

