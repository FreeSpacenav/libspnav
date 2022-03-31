.PHONY: all
all: manual.html

%.html: %.md man_head.html
	cat man_head.html >$@
	markdown $< >>$@
	echo '</body></html>' >>$@
