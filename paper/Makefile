all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' -shell-escape paper

TABLES =

GRAPHS = \
	gen-graph-cumulative-plain.pdf \
	gen-graph-cumulative-33v.pdf \
	gen-graph-cumulative-33ved.pdf \
	gen-graph-cumulative-plain-connected.pdf \
	gen-graph-cumulative-33ve-connected.pdf \
	gen-graph-scatter-plain-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-plain-clique-vs-clique-cilk-t32.pdf \
	gen-graph-scatter-plain-kdown-vs-kdown-par-t32.pdf \
	gen-graph-scatter-plain-kdown-vs-kdown-cilk-t32.pdf \
	gen-graph-scatter-plain-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-plain-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-plain-clique-cilk-t32r-vs-clique-cilk-t32.pdf \
	gen-graph-scatter-plain-kdown-par-t32r-vs-kdown-par-t32.pdf \
	gen-graph-scatter-plain-kdown-cilk-t32r-vs-kdown-cilk-t32.pdf \
	gen-graph-scatter-plain-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33v-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-33v-clique-vs-clique-cilk-t32.pdf \
	gen-graph-scatter-33v-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33v-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-33v-clique-cilk-t32r-vs-clique-cilk-t32.pdf \
	gen-graph-scatter-33v-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33ved-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-clique-vs-clique-cilk-t32.pdf \
	gen-graph-scatter-33ved-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33ved-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-clique-cilk-t32r-vs-clique-cilk-t32.pdf \
	gen-graph-scatter-33ved-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.pdf : graph-%.gnuplot
	gnuplot $<
	sed -i -e '19s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

