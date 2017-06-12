all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape paper

TABLES =

GRAPHS = \
	gen-graph-cumulative-plain.pdf \
	gen-graph-cumulative-33v.pdf \
	gen-graph-cumulative-33ved.pdf \
	gen-graph-cumulative-plain-connected.pdf \
	gen-graph-cumulative-33ve-connected.pdf \
	gen-graph-cumulative-sip.pdf \
	gen-graph-histogram-plain-clique-vs-clique-par-t32.pdf \
	gen-graph-histogram-plain-kdown-vs-kdown-par-t32.pdf \
	gen-graph-histogram-plain-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-histogram-33v-clique-vs-clique-par-t32.pdf \
	gen-graph-histogram-33v-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-histogram-33ved-clique-vs-clique-par-t32.pdf \
	gen-graph-histogram-33ved-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-plain-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-plain-kdown-vs-kdown-par-t32.pdf \
	gen-graph-scatter-plain-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-plain-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-plain-kdown-par-t32r-vs-kdown-par-t32.pdf \
	gen-graph-scatter-plain-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33v-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-33v-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33v-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-33v-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33ved-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33ved-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-sip-kdown-vs-kdown-par-t32.pdf \
	gen-graph-scatter-sip-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-histogram-sip-kdown-vs-kdown-par-t32.pdf \
	gen-graph-histogram-sip-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33ved-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-clique-vs-clique-par-t2.pdf \
	gen-graph-scatter-33ved-clique-par-t2-vs-clique-par-t4.pdf \
	gen-graph-scatter-33ved-clique-par-t4-vs-clique-par-t8.pdf \
	gen-graph-scatter-33ved-clique-par-t8-vs-clique-par-t16.pdf \
	gen-graph-scatter-33ved-clique-par-t16-vs-clique-par-t32.pdf \
	gen-graph-scatter-plain-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-plain-mcsplit-vs-mcsplit-par-t2.pdf \
	gen-graph-scatter-plain-mcsplit-par-t2-vs-mcsplit-par-t4.pdf \
	gen-graph-scatter-plain-mcsplit-par-t4-vs-mcsplit-par-t8.pdf \
	gen-graph-scatter-plain-mcsplit-par-t8-vs-mcsplit-par-t16.pdf \
	gen-graph-scatter-plain-mcsplit-par-t16-vs-mcsplit-par-t32.pdf

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot
	gnuplot $<
	sed -i -e '16,20s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

