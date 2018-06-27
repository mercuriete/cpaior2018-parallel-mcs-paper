# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.5in,1.4in font '\scriptsize' preamble '\input{gnuplot-preamble}'
set output "gen-graph-histogram-33ved-clique-vs-clique-par-t32.tex"

set style line 102 lc rgb '#a0a0a0' lt 1 lw 1
set border ls 102
set colorbox border 102
set key textcolor rgb "black"
set tics textcolor rgb "black"
set label textcolor rgb "black"

load "magma.pal"

set title 'Clique, Lab'

set border 3
set grid
set xtics nomirror out scale 0.2
set ytics nomirror
set key off

min=0.0
max=49.0
n=50
width=(max-min)/n
hist(x,width)=width*floor((0.0+x)/width)+width/2.0
shist(seq,par,width)=((0.0+seq)/(0.0+par)>=max?hist(max,width):hist((0.0+seq)/(0.0+par),width))
fhist(seq,par)=(seq>=1e6||seq<5e2?0:1)

set xrange [min:max+1]

set style fill solid

set xtics add ('${\ge}$50' 50)

plot "../experiments/fatanode-results/mcs33ved/runtimes.data" u (shist($2,$4,width)):(fhist($2,$4)) smooth freq w boxes lc '#812581'

