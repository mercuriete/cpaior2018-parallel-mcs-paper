# vim: set et ft=gnuplot sw=4 :

set terminal tikz standalone color size 1.4in,1.8in font '\tiny'
set output "gen-graph-histogram-plain-kdown-vs-kdown-par-t32.tex"

load "magma.pal"

set title 'k${\downarrow}$, Unlabelled'

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
fhist(seq,par)=(seq>=1e5||seq<5e2?0:1)

set xrange [min:max+1]

set style fill solid

set xtics add ('${\ge}$50' 50)

plot "../experiments/gpgnode-results/mcsplain/runtimes.data" u (shist($4,$16,width)):(fhist($4,$16)) smooth freq w boxes lc '#812581'

