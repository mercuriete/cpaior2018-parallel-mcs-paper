all : tables graphs
	latexmk -pdf -pdflatex='pdflatex -interaction=nonstopmode %O %S' talk

TABLES =

GRAPHS = \
	gen-graph-cumulative-33v.pdf \
	gen-graph-cumulative-33ve-connected.pdf \
	gen-graph-cumulative-33ved.pdf \
	gen-graph-cumulative-plain-connected.pdf \
	gen-graph-cumulative-plain.pdf \
	gen-graph-cumulative-sip.pdf \
	gen-graph-scatter-33v-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-33v-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33ve-connected-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ve-connected-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-33ved-clique-par-t16-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-clique-par-t2-vs-clique-par-t4.pdf \
	gen-graph-scatter-33ved-clique-par-t32r-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-clique-par-t4-vs-clique-par-t8.pdf \
	gen-graph-scatter-33ved-clique-par-t8-vs-clique-par-t16.pdf \
	gen-graph-scatter-33ved-clique-vs-clique-par-t2.pdf \
	gen-graph-scatter-33ved-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-33ved-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-plain-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-plain-connected-clique-vs-clique-par-t32.pdf \
	gen-graph-scatter-plain-connected-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-plain-kdown-par-t32r-vs-kdown-par-t32.pdf \
	gen-graph-scatter-plain-kdown-par-t4-vs-kdown-par-t16.pdf \
	gen-graph-scatter-plain-kdown-vs-kdown-par-t32.pdf \
	gen-graph-scatter-plain-mcsplit-par-t32r-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-plain-mcsplit-par-t4-vs-mcsplit-par-t16.pdf \
	gen-graph-scatter-plain-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-scatter-sip-kdown-vs-kdown-par-t32.pdf \
	gen-graph-scatter-sip-mcsplit-vs-mcsplit-par-t32.pdf \
	gen-graph-histogram-33v-clique-vs-clique-par-t32.pdf \
	gen-graph-histogram-33ve-connected-clique-vs-clique-par-t32.pdf \
	gen-graph-histogram-33ved-clique-vs-clique-par-t32.pdf \
	gen-graph-histogram-plain-clique-vs-clique-par-t32.pdf \
	gen-graph-histogram-plain-connected-clique-vs-clique-par-t32.pdf \
	gen-graph-pp2-33v-clique-vs-clique-par-t32.pdf \
	gen-graph-pp2-33ve-connected-clique-vs-clique-par-t32.pdf \
	gen-graph-pp2-33ved-clique-vs-clique-par-t32.pdf \
	gen-graph-pp2-plain-clique-vs-clique-par-t32.pdf \
	gen-graph-pp2-plain-connected-clique-vs-clique-par-t32.pdf \
	gen-graph-as-clique.pdf \
	gen-graph-as-kdown.pdf \
	gen-graph-as-mcsplit.pdf \

tables : $(TABLES)

graphs : $(GRAPHS)

gen-graph-%.tex : graph-%.gnuplot
	gnuplot $<

gen-graph-%.pdf : graph-%.gnuplot $(DATA)
	gnuplot $<
	sed -i -e '5,20s/^\(\\path.*\)/\% \1/' gen-graph-$*.tex # epic haxx
	latexmk -pdf gen-graph-$*

