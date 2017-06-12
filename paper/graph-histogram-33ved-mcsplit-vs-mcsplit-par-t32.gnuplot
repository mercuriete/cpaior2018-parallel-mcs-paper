# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 2.2in,2.3in font '\footnotesize'
set output "gen-graph-histogram-33ved-mcsplit-vs-mcsplit-par-t32.tex"

load "magma.pal"

set title 'McSplit, Fully labelled'

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
fhist(seq,par)=(seq>=1e5||seq<1e3?0:1)

set xrange [min:max+1]

set xlabel "Speedup"
set ylabel "Number of instances"

set style fill solid

set xtics add ('${\ge}$50' 50)

plot "../experiments/gpgnode-results/mcs33ved/runtimes.data" u (shist($3,$11,width)):(fhist($3,$11)) smooth freq w boxes lc '#812581'

